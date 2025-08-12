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
#include <filesystem>
#include <cstdint>
#include <utility>
#ifdef _OPENMP
#include <omp.h>
#endif

namespace {
// Reasonable task depth before switching to serial recursion to reduce task overhead.
constexpr int k_task_depth_limit = 4;
} // namespace

// -------------------- Checkpoint format PODs --------------------
namespace {
struct CheckpointHeader {
    char magic[8]{'T', 'C', 'H', 'K', 'P', 'T', '1'}; // "TCHKPT1"\0
    uint32_t version{2};
    uint32_t shard_count{0};
    uint64_t game_count{0};
    uint64_t stack_size{0};
    int64_t wall_ms_so_far{0};
};
struct BoardBlob {
    uint32_t occ, black, dame;
    uint8_t player; // duplicated in GameBlob too
    uint8_t pad[3]{};
};
struct GameBlob {
    BoardBlob board;
    uint8_t player_to_move;
    uint8_t looping; // 1 if looping terminal
    uint16_t reserved{0};
    uint32_t history_len; // number of size_t entries (stored as 64-bit if version>=2, else legacy 32-bit)
};
struct FrameBlob {
    GameBlob game;
    uint32_t next_idx;
};
static void write_blob(FILE* f, const void* p, size_t n) { std::fwrite(p, 1, n, f); }
static bool read_blob(FILE* f, void* p, size_t n) { return std::fread(p, 1, n, f) == n; }
} // namespace

// -------------------- Iterative traversal helpers --------------------
void Traversal::emit_result(const Game& game) {
    const auto& history = game.get_move_sequence();
    const std::size_t move_history_count = history.size() / 2;
    if (game.is_looping()) record_loop(game.board().hash());
    const std::size_t total = game_count.fetch_add(1, std::memory_order_relaxed);
    std::function<void(const ResultEvent&)> cb_copy;
    {
        std::scoped_lock lock(cb_mutex_);
        cb_copy = result_cb_;
    }
    if (!cb_copy) return;
    std::vector<std::size_t> moves_only;
    moves_only.reserve(move_history_count);
    for (std::size_t i = 1; i < history.size(); i += 2) moves_only.push_back(history[i]);
    ResultEvent ev{
        .game_id = total,
        .looping = game.is_looping(),
        .winner = game.is_looping() ? std::nullopt
                                    : std::optional<PieceColor>(
                                          (game.player() == PieceColor::BLACK) ? PieceColor::WHITE : PieceColor::BLACK),
        .move_history = std::move(moves_only),
        .history = history,
    };
    cb_copy(ev);
}

void Traversal::traverse_iterative(Game root) {
    work_stack_.clear();
    work_stack_.push_back(Frame{std::move(root), 0});
    while (!work_stack_.empty() && !stop_.load(std::memory_order_relaxed)) {
        Frame& f = work_stack_.back();
        const auto h = f.game.board().hash();
        if (loop_seen(h)) {
            work_stack_.pop_back();
            continue;
        }
        const auto mc = f.game.move_count();
        if (mc == 0) {
            emit_result(f.game);
            work_stack_.pop_back();
            continue;
        }
        if (f.next_idx >= mc) {
            work_stack_.pop_back();
            continue;
        }
        const auto idx = f.next_idx++;
        Game child = f.game; // copy
        child.select_move(idx);
        work_stack_.push_back(Frame{std::move(child), 0});
    }
}

void Traversal::start_root_only(Game root) {
    work_stack_.clear();
    work_stack_.push_back(Frame{std::move(root), 0});
}

bool Traversal::step_one() {
    if (work_stack_.empty()) return false;
    if (stop_.load(std::memory_order_relaxed)) return false;
    Frame& f = work_stack_.back();
    const auto h = f.game.board().hash();
    if (loop_seen(h)) {
        work_stack_.pop_back();
        return !work_stack_.empty();
    }
    const auto mc = f.game.move_count();
    if (mc == 0) {
        emit_result(f.game);
        work_stack_.pop_back();
        return !work_stack_.empty();
    }
    if (f.next_idx >= mc) {
        work_stack_.pop_back();
        return !work_stack_.empty();
    }
    const auto idx = f.next_idx++;
    Game child = f.game;
    child.select_move(idx);
    work_stack_.push_back(Frame{std::move(child), 0});
    return true;
}

void Traversal::run_from_work_stack() {
    while (!work_stack_.empty() && !stop_.load(std::memory_order_relaxed)) {
        Frame& f = work_stack_.back();
        const auto h = f.game.board().hash();
        if (loop_seen(h)) {
            work_stack_.pop_back();
            continue;
        }
        const auto mc = f.game.move_count();
        if (mc == 0) {
            emit_result(f.game);
            work_stack_.pop_back();
            continue;
        }
        if (f.next_idx >= mc) {
            work_stack_.pop_back();
            continue;
        }
        const auto idx = f.next_idx++;
        Game child = f.game;
        child.select_move(idx);
        work_stack_.push_back(Frame{std::move(child), 0});
    }
}

bool Traversal::save_checkpoint(const std::string& path) const {
    const std::string tmp = path + ".tmp";
    FILE* f = std::fopen(tmp.c_str(), "wb");
    if (!f) return false;
    CheckpointHeader H{};
    H.shard_count = static_cast<uint32_t>(loop_shards_.size());
    H.game_count = game_count.load(std::memory_order_relaxed);
    H.stack_size = work_stack_.size();
    if (start_tp_.time_since_epoch().count() != 0) {
        const auto now = std::chrono::steady_clock::now();
        H.wall_ms_so_far = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_tp_).count();
    }
    write_blob(f, &H, sizeof(H));
    for (const auto& fr : work_stack_) {
        FrameBlob fb{};
        fb.game.board.occ = fr.game.board().occ_bits();
        fb.game.board.black = fr.game.board().black_bits();
        fb.game.board.dame = fr.game.board().dame_bits();
        fb.game.board.player = static_cast<uint8_t>(fr.game.player());
        fb.game.player_to_move = fb.game.board.player;
        fb.game.looping = fr.game.is_looping() ? 1 : 0;
        const auto& hist = fr.game.get_move_sequence();
        fb.game.history_len = static_cast<uint32_t>(hist.size());
        fb.next_idx = fr.next_idx;
        write_blob(f, &fb, sizeof(fb));
        for (auto v : hist) {
            uint64_t u = static_cast<uint64_t>(v);
            write_blob(f, &u, sizeof(u));
        }
    }
    for (const auto& shard : loop_shards_) {
        const uint64_t n = shard.set.size();
        write_blob(f, &n, sizeof(n));
        for (auto h : shard.set) {
            uint64_t hv = static_cast<uint64_t>(h);
            write_blob(f, &hv, sizeof(hv));
        }
    }
    std::fclose(f);
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp);
        return false;
    }
    return true;
}

bool Traversal::load_checkpoint(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    CheckpointHeader H{};
    if (!read_blob(f, &H, sizeof(H))) {
        std::fclose(f);
        return false;
    }
    if (std::string_view(H.magic, 7) != "TCHKPT1") {
        std::fclose(f);
        return false;
    }
    work_stack_.clear();
    work_stack_.reserve(H.stack_size);
    for (uint64_t i = 0; i < H.stack_size; ++i) {
        FrameBlob fb{};
        if (!read_blob(f, &fb, sizeof(fb))) {
            std::fclose(f);
            return false;
        }
        Game g;
        Board b;
        b.set_from_masks(fb.game.board.occ, fb.game.board.black, fb.game.board.dame);
        g.set_board(b);
        g.set_player(static_cast<PieceColor>(fb.game.player_to_move));
        g.set_looping(fb.game.looping != 0);
        std::vector<std::size_t> hist;
        hist.resize(fb.game.history_len);
        if (H.version >= 2) {
            for (uint32_t k = 0; k < fb.game.history_len; ++k) {
                uint64_t v;
                if (!read_blob(f, &v, sizeof(v))) {
                    std::fclose(f);
                    return false;
                }
                hist[k] = static_cast<std::size_t>(v);
            }
        } else { // legacy v1 (32-bit truncation)
            for (uint32_t k = 0; k < fb.game.history_len; ++k) {
                uint32_t v;
                if (!read_blob(f, &v, sizeof(v))) {
                    std::fclose(f);
                    return false;
                }
                hist[k] = static_cast<std::size_t>(v);
            }
        }
        g.set_move_sequence(std::move(hist));
        work_stack_.push_back(Frame{std::move(g), fb.next_idx});
    }
    for (auto& shard : loop_shards_) {
        uint64_t n = 0;
        if (!read_blob(f, &n, sizeof(n))) {
            std::fclose(f);
            return false;
        }
        std::unique_lock lk(shard.mtx);
        shard.set.clear();
        for (uint64_t i = 0; i < n; ++i) {
            uint64_t hv;
            if (!read_blob(f, &hv, sizeof(hv))) {
                std::fclose(f);
                return false;
            }
            shard.set.insert(static_cast<std::size_t>(hv));
        }
    }
    game_count.store(H.game_count, std::memory_order_relaxed);
    std::fclose(f);
    start_tp_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(H.wall_ms_so_far);
    return true;
}

void Traversal::resume_or_start(const std::string& chk_path, Game root) {
    stop_.store(false, std::memory_order_relaxed);
    if (!chk_path.empty() && load_checkpoint(chk_path)) {
        run_from_work_stack();
    } else {
        game_count.store(1, std::memory_order_relaxed);
        clear_loops();
        start_tp_ = std::chrono::steady_clock::now();
        traverse_iterative(std::move(root));
    }
    print_summary();
}

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
