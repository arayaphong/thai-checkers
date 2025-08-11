#include "Traversal.h"
#include <format>
#include <ranges>
#include <string>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/resource.h>
#include <thread>
#ifdef _OPENMP
#include <omp.h>
#endif

namespace {
// Reasonable task depth before switching to serial recursion to reduce task overhead.
constexpr int k_task_depth_limit = 4;
} // namespace

bool Traversal::loop_seen(std::size_t h) const {
    const auto shard = shard_for(h);
    const auto& s = loop_shards_[shard];
    std::shared_lock lk(s.mtx);
    return s.set.contains(h);
}

void Traversal::record_loop(std::size_t h) {
    const auto shard = shard_for(h);
    auto& s = loop_shards_[shard];
    std::unique_lock lk(s.mtx);
    s.set.insert(h);
}

void Traversal::clear_loops() {
    for (auto& shard : loop_shards_) {
        std::unique_lock lk(shard.mtx);
        shard.set.clear();
    }
}

void Traversal::reset() {
    request_stop();
    clear_loops();
    // Reset counters and timers
    game_count.store(1, std::memory_order_relaxed);
    stop_.store(false, std::memory_order_relaxed);
    start_tp_ = {};
    deadline_tp_ = {};
}

void Traversal::traverse_impl(const Game& game, int depth) {
    // Honor stop signal early
    if (stop_.load(std::memory_order_relaxed)) return;

    const std::size_t move_count = game.move_count();

    // Fast-path check in sharded loop DB
    const auto h = game.board().hash();
    if (loop_seen(h)) return;

    if (move_count == 0) {
        const auto& history = game.get_move_sequence();
        const std::size_t move_history_count = history.size() / 2;

        if (game.is_looping()) record_loop(h);

        // Emit result callback (if any) with move history and total count
        const std::size_t total = game_count.fetch_add(1, std::memory_order_relaxed);
        std::function<void(const ResultEvent&)> cb_copy;
        {
            std::scoped_lock lock(cb_mutex_);
            cb_copy = result_cb_;
        }
        if (cb_copy) {
            // Copy only the chosen move indices portion of history
            std::vector<std::size_t> moves_only;
            moves_only.reserve(move_history_count);
            // board_move_sequence alternates: push initial hash, then for each ply push move index and new board hash
            // From Game::select_move it pushes index, and execute_move pushes new board hash
            for (std::size_t i = 1; i < history.size(); i += 2) moves_only.push_back(history[i]);

            ResultEvent ev{
                .game_id = total,
                .looping = game.is_looping(),
                .winner = game.is_looping()
                              ? std::nullopt
                              : std::optional<PieceColor>((game.player() == PieceColor::BLACK) ? PieceColor::WHITE
                                                                                               : PieceColor::BLACK),
                .move_history = std::move(moves_only),
                .history = history, // copy raw full history
            };
            cb_copy(ev);
        }
        return;
    }

#ifdef _OPENMP
    if (depth < k_task_depth_limit) {
        if (omp_in_parallel()) {
            for (std::size_t i = 0; i < move_count; ++i) {
                if (stop_.load(std::memory_order_relaxed)) break;
                const Game base = game;
#pragma omp task firstprivate(base, i)
                {
                    if (!stop_.load(std::memory_order_relaxed)) {
                        Game next_game = base;
                        next_game.select_move(i);
                        traverse_impl(next_game, depth + 1);
                    }
                }
            }
        } else {
#pragma omp parallel
            {
#pragma omp single nowait
                {
                    for (std::size_t i = 0; i < move_count; ++i) {
                        if (stop_.load(std::memory_order_relaxed)) break;
                        const Game base = game;
#pragma omp task firstprivate(base, i)
                        {
                            if (!stop_.load(std::memory_order_relaxed)) {
                                Game next_game = base;
                                next_game.select_move(i);
                                traverse_impl(next_game, depth + 1);
                            }
                        }
                    }
                }
            }
        }
    } else
#endif
    {
        // Serial fallback to reduce task creation overhead at deeper levels
        for (std::size_t i = 0; i < move_count; ++i) {
            if (stop_.load(std::memory_order_relaxed)) break;
            Game next_game = game;
            next_game.select_move(i);
            traverse_impl(next_game, depth + 1);
        }
    }
}

void Traversal::traverse(const Game& game) {
    // Fresh run state, similar to traverse_for but without a deadline
    stop_.store(false, std::memory_order_relaxed);
    game_count.store(1, std::memory_order_relaxed);
    clear_loops();
    start_tp_ = std::chrono::steady_clock::now();
    deadline_tp_ = {};

    // Progress thread (no deadline, only emits progress callbacks)
    std::thread progress_thr([this]() {
        static thread_local auto last = std::chrono::steady_clock::now();
        while (!stop_.load(std::memory_order_relaxed)) {
            const auto now = std::chrono::steady_clock::now();
            if ((now - last) >= progress_interval_ms_) {
                ProgressEvent ev{.games = game_count.load(std::memory_order_relaxed) - 1};
                std::function<void(const ProgressEvent&)> cb_copy;
                {
                    std::scoped_lock lock(cb_mutex_);
                    cb_copy = progress_cb_;
                }
                if (cb_copy) cb_copy(ev);
                last = now;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    // Run traversal to completion
    traverse_impl(game, /*depth=*/0);

    // Signal progress thread to stop and join
    stop_.store(true, std::memory_order_relaxed);
    if (progress_thr.joinable()) progress_thr.join();

    // Emit summary (via callback or stdout fallback)
    print_summary();
}

void Traversal::traverse_for(std::chrono::milliseconds duration, const Game& game) {
    // fresh run state
    stop_.store(false, std::memory_order_relaxed);
    game_count.store(1, std::memory_order_relaxed);
    clear_loops();
    start_tp_ = std::chrono::steady_clock::now();
    deadline_tp_ = start_tp_ + duration;

    // Start a watchdog thread that flips stop_ at deadline and emits progress periodically
    std::thread watchdog([this]() {
        while (!stop_.load(std::memory_order_relaxed)) {
            if (std::chrono::steady_clock::now() >= deadline_tp_) {
                stop_.store(true, std::memory_order_relaxed);
                break;
            }
            static thread_local auto last = std::chrono::steady_clock::now();
            const auto now = std::chrono::steady_clock::now();
            if ((now - last) >= progress_interval_ms_) {
                ProgressEvent ev{.games = game_count.load(std::memory_order_relaxed) - 1};
                // Copy cb under lock to avoid races
                std::function<void(const ProgressEvent&)> cb_copy;
                {
                    std::scoped_lock lock(cb_mutex_);
                    cb_copy = progress_cb_;
                }
                if (cb_copy) cb_copy(ev);
                last = now;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    // Run traversal (may spawn OpenMP tasks internally)
    traverse_impl(game, 0);

    // Ensure stop flag is set and join watchdog
    stop_.store(true, std::memory_order_relaxed);
    if (watchdog.joinable()) watchdog.join();

    print_summary();
}

// Read Linux /proc/self/status for memory stats (VmRSS, VmHWM, etc.)
long Traversal::proc_status_kb(const char* key) {
    FILE* f = std::fopen("/proc/self/status", "r");
    if (!f) return -1;
    char line[512];
    const size_t keylen = std::strlen(key);
    long val = -1;
    while (std::fgets(line, sizeof(line), f)) {
        if (std::strncmp(line, key, keylen) == 0) {
            // Format: Key:\t<value> kB
            char* p = line + keylen;
            while (*p && (*p < '0' || *p > '9')) ++p;
            val = std::strtol(p, nullptr, 10);
            break;
        }
    }
    std::fclose(f);
    return val; // in kB
}

double Traversal::process_cpu_seconds() {
    struct rusage ru{};
    if (getrusage(RUSAGE_SELF, &ru) != 0) return -1.0;
    const double user = ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1e6;
    const double sys = ru.ru_stime.tv_sec + ru.ru_stime.tv_usec / 1e6;
    return user + sys;
}

void Traversal::print_summary() const {
    const auto end_tp = std::chrono::steady_clock::now();
    const auto wall_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_tp - start_tp_).count();
    const std::size_t games = game_count.load(std::memory_order_relaxed) - 1; // ids started at 1
    const double secs = wall_ms / 1000.0;
    const double cps = secs > 0.0 ? (games / secs) : 0.0;
    const double cpu_s = process_cpu_seconds();
    const long rss_kb = proc_status_kb("VmRSS:");
    const long hwm_kb = proc_status_kb("VmHWM:");

    std::function<void(const SummaryEvent&)> cb_copy;
    {
        std::scoped_lock lock(cb_mutex_);
        cb_copy = summary_cb_;
    }
    if (cb_copy) {
        SummaryEvent ev{
            .wall_seconds = secs,
            .games = games,
            .throughput_games_per_sec = cps,
            .cpu_seconds = cpu_s,
            .cpu_util_percent = (secs > 0.0 && cpu_s >= 0.0) ? (cpu_s / secs) * 100.0 : -1.0,
            .rss_kb = rss_kb,
            .hwm_kb = hwm_kb,
        };
        cb_copy(ev);
    } else {
        std::scoped_lock lk(io_mutex);
        std::cout << "Summary: wall=" << secs << "s, games=" << games << ", throughput=" << cps << "/s, cpu_s=" << cpu_s
                  << ", cpu_util=" << ((secs > 0.0 && cpu_s >= 0.0) ? (cpu_s / secs) * 100.0 : -1.0)
                  << "%, rss_kb=" << rss_kb << ", hwm_kb=" << hwm_kb << '\n';
    }
}
