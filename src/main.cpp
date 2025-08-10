#include <chrono>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <vector>
#include <string>
#include <sstream>
#include <atomic>
#include <cstdint>
#include <functional>
#include <sys/resource.h>
#include <unistd.h>

#include "Game.h"

namespace
{
    struct usage_snapshot
    {
        double user_seconds{0.0};
        double system_seconds{0.0};
        long max_rss_kb{0};
        size_t current_rss_kb{0};
    };

    // Global flag to control verbosity for tests/benchmarks
    bool g_quiet = false;

    usage_snapshot capture_usage()
    {
        usage_snapshot snap;
        rusage ru{};
        if (getrusage(RUSAGE_SELF, &ru) == 0)
        {
            snap.user_seconds = static_cast<double>(ru.ru_utime.tv_sec) + ru.ru_utime.tv_usec / 1'000'000.0;
            snap.system_seconds = static_cast<double>(ru.ru_stime.tv_sec) + ru.ru_stime.tv_usec / 1'000'000.0;
            // On Linux ru_maxrss is in kilobytes
            snap.max_rss_kb = ru.ru_maxrss;
        }
        // Get current resident set size (in KB)
        std::ifstream statm("/proc/self/statm");
        if (statm)
        {
            size_t size_pages = 0, resident_pages = 0;
            statm >> size_pages >> resident_pages; // we only need the first two
            long page_kb = sysconf(_SC_PAGESIZE) / 1024;
            snap.current_rss_kb = resident_pages * static_cast<size_t>(page_kb);
        }
        return snap;
    }
} // namespace

// Global RNG with optional seeding for reproducibility
namespace
{
    // Thread-local RNG to be safe under OpenMP parallel regions
    thread_local std::mt19937 g_rng{std::random_device{}()};
    // Global seed value set via CLI (0 means no override)
    std::uint32_t g_seed_value = 0;
    bool g_has_seed = false;

    inline void set_global_seed(std::uint32_t seed)
    {
        g_seed_value = seed;
        g_has_seed = true;
        // Also seed current thread's RNG for single-threaded runs
        g_rng.seed(seed);
    }

    inline void seed_thread_rng_with_tid(int tid)
    {
        if (g_has_seed)
        {
            // Mix in thread id using golden ratio constant for decorrelation
            const std::uint32_t mixed = g_seed_value ^ (0x9E3779B9u * static_cast<std::uint32_t>(tid + 1));
            g_rng.seed(mixed);
        }
        // else keep random_device-based default seeding
    }
}

void random_game_play()
{
    Game game;
    int state = 0;

    while (true)
    {
        const char *current_player = (state % 2 == 0) ? "White" : "Black";
        if (!g_quiet)
        {
            std::cout << "[State " << state << "] " << current_player << " Turn:" << std::endl;
            game.print_board();
            std::cout << std::endl;
        }

        const std::size_t choices = game.move_count();
        if (choices == 0)
        {
            std::cout << "No more moves available. Game over!" << std::endl;
            const char *winner = (state % 2 == 0) ? "Black" : "White";
            std::cout << "Victory: " << winner << " wins!" << std::endl;
            break;
        }

        game.print_choices();

        // Randomly select an available move for demonstration (seedable via --seed)
        std::uniform_int_distribution<std::size_t> dist(0, choices - 1);
        game.select_move(dist(g_rng));

        ++state;
    }

    const auto &move_sequence = game.get_move_sequence();
    std::cout << "Move history: ";
    for (std::size_t i = 1; i < move_sequence.size(); i += 2)
    {
        std::cout << move_sequence[i] << " ";
    }
    std::cout << std::endl;
}

void random_game_play_quiet()
{
    Game game;
    while (true)
    {
        const std::size_t move_count = game.move_count();
        if (move_count == 0)
        {
            break;
        }

        std::uniform_int_distribution<std::size_t> dist(0, move_count - 1);
        game.select_move(dist(g_rng));
    }
}

// ============================
// Parallel, resumable traversal
// ============================
namespace
{
    struct traverse_options
    {
        int split_depth = 5;                // depth to split the tree into a frontier
        std::uint64_t start_offset = 0;     // number of frontier seeds to skip (resume cursor)
        std::uint64_t max_frontier = 0;     // 0 = process all remaining frontier seeds
        std::string seed_token;             // Alternate encoding of resume cursor: "D:OFFSET"
        bool verbose = false;               // print progress
    int max_depth = 200;                // safety cap for DFS to prevent cycles/infinite games
    // Optional limits to bound traversal work/time per run
    std::chrono::milliseconds time_limit{0}; // 0 = no time limit
    std::uint64_t node_limit = 0;            // 0 = unlimited nodes
    std::uint64_t leaf_limit = 0;            // 0 = unlimited leaves
    };

    struct traverse_result
    {
        std::uint64_t total_frontier = 0;   // total frontier size at split_depth (for reference)
        std::uint64_t processed_seeds = 0;  // how many frontier seeds processed in this run
        std::uint64_t next_offset = 0;      // cursor to resume from next time
        std::uint64_t nodes_visited = 0;    // internal nodes expanded in post-frontier DFS
        std::uint64_t terminal_positions = 0; // number of terminal leaves reached (complete games)
        int max_depth_reached = 0;          // maximum depth from root reached during this run
        std::string resume_token;           // formatted as "D:OFFSET"
    };

    // Parse seed token of the form "D:OFFSET" or just "OFFSET" (uses default/current split_depth)
    inline void apply_seed_token(const std::string &token, int &split_depth, std::uint64_t &start_offset)
    {
        if (token.empty())
            return;
        const auto pos = token.find(':');
        if (pos == std::string::npos)
        {
            // Only offset present
            start_offset = std::strtoull(token.c_str(), nullptr, 10);
            return;
        }
        const std::string d = token.substr(0, pos);
        const std::string o = token.substr(pos + 1);
        if (!d.empty())
            split_depth = std::atoi(d.c_str());
        if (!o.empty())
            start_offset = std::strtoull(o.c_str(), nullptr, 10);
    }

    // Replay a path of child indices into a fresh Game from the initial position.
    // Returns false if the path is invalid under the current move ordering.
    inline bool replay_path(Game &g, const std::vector<std::size_t> &path, int &max_depth_reached)
    {
        int depth = 0;
        for (std::size_t idx : path)
        {
            const std::size_t choices = g.move_count();
            if (choices == 0 || idx >= choices)
                return false;
            g.select_move(idx);
            ++depth;
        }
        if (depth > max_depth_reached)
            max_depth_reached = depth;
        return true;
    }

    // Enumerate frontier seeds up to split_depth, deterministically in generation order.
    // We do a DFS from root, and emit the path when reaching split_depth or terminal position.
    void enumerate_frontier(const Game &base,
                            int split_depth,
                            std::uint64_t skip,
                            std::uint64_t limit, // 0 for unlimited
                            std::vector<std::vector<std::size_t>> &out,
                            std::uint64_t &total_frontier_count)
    {
        out.clear();
        total_frontier_count = 0;
        std::vector<std::size_t> path;
        bool stop = false;

        // Recurse over copies so that we don't need undo mechanics
        std::function<void(const Game &, int)> dfs = [&](const Game &g, int depth) {
            if (stop)
                return;
            const std::size_t choices = g.move_count();
            if (choices == 0 || depth == split_depth)
            {
                // This partial path is a frontier element
                ++total_frontier_count;
                if (total_frontier_count > skip && (limit == 0 || out.size() < limit))
                {
                    out.push_back(path);
                }
                if (limit != 0 && out.size() >= limit)
                {
                    // We've collected enough; compute a conservative total and stop globally
                    total_frontier_count = skip + out.size();
                    stop = true;
                }
                return;
            }
            for (std::size_t i = 0; i < choices; ++i)
            {
                if (stop)
                    break;
                Game next = g; // copy current state
                next.select_move(i);
                path.push_back(i);
                dfs(next, depth + 1);
                path.pop_back();
            }
        };

        dfs(base, 0);
    }

    // Depth-first traversal to terminal states, counting nodes and leaves.
    void dfs_to_terminal(Game &g,
                         std::uint64_t &nodes,
                         std::uint64_t &leaves,
                         int current_depth,
                         int &max_depth_reached,
                         int max_depth_limit,
                         const std::chrono::steady_clock::time_point *deadline,
                         std::atomic<bool> *stop,
                         const std::uint64_t node_cap,
                         const std::uint64_t leaf_cap)
    {
        if (stop && stop->load(std::memory_order_relaxed)) return;
        if (deadline && std::chrono::steady_clock::now() >= *deadline)
        {
            if (stop) stop->store(true, std::memory_order_relaxed);
            return;
        }
        const std::size_t choices = g.move_count();
        if (choices == 0)
        {
            ++leaves;
            if (current_depth > max_depth_reached)
                max_depth_reached = current_depth;
            if ((leaf_cap && leaves >= leaf_cap) || (stop && stop->load()))
            {
                if (stop) stop->store(true, std::memory_order_relaxed);
                return;
            }
            return;
        }
        if (max_depth_limit > 0 && current_depth >= max_depth_limit)
        {
            // Reached depth cap: treat as a truncated leaf
            ++leaves;
            if (current_depth > max_depth_reached)
                max_depth_reached = current_depth;
            if ((leaf_cap && leaves >= leaf_cap) || (stop && stop->load()))
            {
                if (stop) stop->store(true, std::memory_order_relaxed);
                return;
            }
            return;
        }
        ++nodes; // count this expansion
        if ((node_cap && nodes >= node_cap))
        {
            if (stop) stop->store(true, std::memory_order_relaxed);
            return;
        }
        for (std::size_t i = 0; i < choices; ++i)
        {
            if (stop && stop->load(std::memory_order_relaxed)) return;
            if (deadline && std::chrono::steady_clock::now() >= *deadline)
            {
                if (stop) stop->store(true, std::memory_order_relaxed);
                return;
            }
            Game next = g;
            next.select_move(i);
            dfs_to_terminal(next, nodes, leaves, current_depth + 1, max_depth_reached, max_depth_limit, deadline, stop, node_cap, leaf_cap);
        }
    }

    traverse_result traverse_impl(const traverse_options &opts)
    {
        traverse_result res{};

        int split_depth = opts.split_depth;
        std::uint64_t start_offset = opts.start_offset;
        if (!opts.seed_token.empty())
        {
            apply_seed_token(opts.seed_token, split_depth, start_offset);
        }

        Game base;

        // Build a frontier window starting at offset, with an optional max size
        std::vector<std::vector<std::size_t>> frontier;
        enumerate_frontier(base, split_depth, start_offset, opts.max_frontier, frontier, res.total_frontier);

        const std::size_t N = frontier.size();
        if (opts.verbose)
        {
            std::cout << "Frontier split_depth=" << split_depth
                      << ", built window size=" << N
                      << ", total_frontier≥" << res.total_frontier
                      << ", start_offset=" << start_offset << "\n";
        }

    std::atomic<std::uint64_t> nodes{0};
    std::atomic<std::uint64_t> leaves{0};
    std::atomic<std::uint64_t> seeds_done{0};
        int max_depth_reached = 0;
        std::atomic<bool> stop{false};
        std::chrono::steady_clock::time_point deadline;
        const std::chrono::steady_clock::time_point *deadline_ptr = nullptr;
        if (opts.time_limit.count() > 0)
        {
            deadline = std::chrono::steady_clock::now() + opts.time_limit;
            deadline_ptr = &deadline;
        }

#ifdef _OPENMP
#pragma omp parallel
        {
            // Per-thread local counters to minimize atomic contention
            std::uint64_t local_nodes = 0;
            std::uint64_t local_leaves = 0;
            int local_max_depth = 0;

#pragma omp for schedule(dynamic)
        for (std::int64_t i = 0; i < static_cast<std::int64_t>(N); ++i)
            {
                Game g;
                if (!replay_path(g, frontier[static_cast<std::size_t>(i)], local_max_depth))
                    continue; // invalid under current rules; skip

                if (!stop.load(std::memory_order_relaxed))
                {
                    dfs_to_terminal(g, local_nodes, local_leaves,
                                    static_cast<int>(frontier[static_cast<std::size_t>(i)].size()),
                                    local_max_depth, opts.max_depth,
                                    deadline_ptr, &stop,
                                    opts.node_limit ? (opts.node_limit - nodes.load(std::memory_order_relaxed)) : 0,
                                    opts.leaf_limit ? (opts.leaf_limit - leaves.load(std::memory_order_relaxed)) : 0);
            seeds_done.fetch_add(1, std::memory_order_relaxed);
                }
            }

            nodes.fetch_add(local_nodes, std::memory_order_relaxed);
            leaves.fetch_add(local_leaves, std::memory_order_relaxed);
#pragma omp critical
            {
                if (local_max_depth > max_depth_reached)
                    max_depth_reached = local_max_depth;
            }
        }
#else
        {
        std::uint64_t seq_nodes = 0;
        std::uint64_t seq_leaves = 0;
        std::uint64_t seq_done = 0;
            for (std::size_t i = 0; i < N; ++i)
            {
                Game g;
                if (!replay_path(g, frontier[i], max_depth_reached))
                    continue;
                if (!stop.load(std::memory_order_relaxed))
                {
                    dfs_to_terminal(g, seq_nodes, seq_leaves,
                                    static_cast<int>(frontier[i].size()), max_depth_reached,
                                    opts.max_depth, deadline_ptr, &stop,
                                    opts.node_limit ? (opts.node_limit - seq_nodes) : 0,
                                    opts.leaf_limit ? (opts.leaf_limit - seq_leaves) : 0);
            ++seq_done;
                }
                if (stop.load(std::memory_order_relaxed)) break;
            }
            nodes.store(seq_nodes, std::memory_order_relaxed);
            leaves.store(seq_leaves, std::memory_order_relaxed);
        seeds_done.store(seq_done, std::memory_order_relaxed);
        }
#endif

        res.nodes_visited = nodes.load();
        res.terminal_positions = leaves.load();
        res.max_depth_reached = max_depth_reached;
    res.processed_seeds = seeds_done.load(std::memory_order_relaxed);
    res.next_offset = start_offset + res.processed_seeds;
        {
            std::ostringstream oss;
            oss << split_depth << ":" << res.next_offset;
            res.resume_token = oss.str();
        }
        return res;
    }
} // namespace

int smoke_test(int argc, char **argv)
{
    std::chrono::seconds test_duration{10};
    // Very small, simple CLI: --seconds N, -s N, --quiet
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--quiet") == 0 || std::strcmp(argv[i], "-q") == 0)
        {
            g_quiet = true;
        }
        else if ((std::strcmp(argv[i], "--seconds") == 0 || std::strcmp(argv[i], "-s") == 0) && i + 1 < argc)
        {
            const int n = std::atoi(argv[++i]);
            if (n > 0)
                test_duration = std::chrono::seconds{n};
        }
    else if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
        {
            const unsigned long long seed = std::strtoull(argv[++i], nullptr, 10);
            set_global_seed(static_cast<std::uint32_t>(seed));
        }
    }
    const auto wall_start = std::chrono::steady_clock::now();
    const auto usage_start = capture_usage();
    std::size_t games_played = 0;

    if (g_quiet)
    {
#ifdef _OPENMP
        // Parallelize quiet mode where no printing occurs.
        const auto start_time = wall_start;
        const auto duration = test_duration;
        std::size_t total_games = 0;
#pragma omp parallel
        {
            seed_thread_rng_with_tid(omp_get_thread_num());
            std::size_t local_count = 0;
            while (std::chrono::steady_clock::now() - start_time < duration)
            {
                random_game_play_quiet();
                ++local_count;
            }
#pragma omp atomic update
            total_games += local_count;
        }
        games_played = total_games;
#else
        while (std::chrono::steady_clock::now() - wall_start < test_duration)
        {
            random_game_play_quiet();
            ++games_played;
        }
#endif
    }
    else
    {
        // Keep verbose mode single-threaded to avoid interleaved output
        while (std::chrono::steady_clock::now() - wall_start < test_duration)
        {
            random_game_play();
            ++games_played;
        }
    }

    const auto wall_end = std::chrono::steady_clock::now();
    const auto usage_end = capture_usage();

    const auto wall_seconds = std::chrono::duration<double>(wall_end - wall_start).count();
    const double cpu_user = usage_end.user_seconds - usage_start.user_seconds;
    const double cpu_system = usage_end.system_seconds - usage_start.system_seconds;
    const double cpu_total = cpu_user + cpu_system;
    const double games_per_second = wall_seconds > 0.0 ? games_played / wall_seconds : 0.0;
    const double cpu_util_percent = wall_seconds > 0.0 ? (cpu_total / wall_seconds) * 100.0 : 0.0;

    std::cout << "\n==== Profiling Summary ====\n";
    std::cout << "Wall time           : " << wall_seconds << " s\n";
    std::cout << "Games played        : " << games_played << "\n";
    std::cout << "Games / second      : " << games_per_second << "\n";
    std::cout << "CPU user time       : " << cpu_user << " s\n";
    std::cout << "CPU system time     : " << cpu_system << " s\n";
    std::cout << "CPU total time      : " << cpu_total << " s\n";
    std::cout << "CPU utilization     : " << cpu_util_percent << " % (total CPU time / wall)\n";
    std::cout << "Current RSS         : " << usage_end.current_rss_kb << " KB\n";
    std::cout << "Max RSS (ru_maxrss) : " << usage_end.max_rss_kb << " KB\n";
    std::cout << "===========================\n";
    if (!g_quiet)
    {
        std::cout << "Note: Extensive per-move printing will heavily reduce games/second.\n";
    }

    return 0;
}



int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
    // Parse new traversal CLI first; if present, run traversal mode and exit.
    bool do_traverse = false;
    traverse_options topts{};

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--traverse") == 0)
        {
            do_traverse = true;
        }
        else if ((std::strcmp(argv[i], "--split-depth") == 0 || std::strcmp(argv[i], "-d") == 0) && i + 1 < argc)
        {
            topts.split_depth = std::atoi(argv[++i]);
            if (topts.split_depth < 0) topts.split_depth = 0;
        }
        else if (std::strcmp(argv[i], "--cursor") == 0 && i + 1 < argc)
        {
            topts.start_offset = std::strtoull(argv[++i], nullptr, 10);
        }
        else if ((std::strcmp(argv[i], "--limit") == 0 || std::strcmp(argv[i], "-n") == 0) && i + 1 < argc)
        {
            topts.max_frontier = std::strtoull(argv[++i], nullptr, 10);
        }
        else if (std::strcmp(argv[i], "--resume-token") == 0 && i + 1 < argc)
        {
            topts.seed_token = argv[++i];
        }
        else if (std::strcmp(argv[i], "--verbose") == 0 || std::strcmp(argv[i], "-v") == 0)
        {
            topts.verbose = true;
        }
        else if ((std::strcmp(argv[i], "--max-depth") == 0 || std::strcmp(argv[i], "-m") == 0) && i + 1 < argc)
        {
            topts.max_depth = std::atoi(argv[++i]);
            if (topts.max_depth < 0) topts.max_depth = 0; // 0 disables the cap
        }
        else if (std::strcmp(argv[i], "--time-limit-ms") == 0 && i + 1 < argc)
        {
            const long long ms = std::strtoll(argv[++i], nullptr, 10);
            if (ms > 0) topts.time_limit = std::chrono::milliseconds(ms);
        }
        else if (std::strcmp(argv[i], "--node-limit") == 0 && i + 1 < argc)
        {
            topts.node_limit = std::strtoull(argv[++i], nullptr, 10);
        }
        else if (std::strcmp(argv[i], "--leaf-limit") == 0 && i + 1 < argc)
        {
            topts.leaf_limit = std::strtoull(argv[++i], nullptr, 10);
        }
    }

    if (do_traverse)
    {
        // Provide sane defaults if user gave no bounds to avoid long runs
        if (topts.max_frontier == 0 && topts.time_limit.count() == 0 && topts.node_limit == 0 && topts.leaf_limit == 0)
        {
            topts.max_frontier = 100; // process at most 100 frontier seeds per run by default
        }
        const auto res = traverse_impl(topts);
        std::cout << "\n==== Traverse Summary ====\n";
        std::cout << "Split depth         : " << topts.split_depth << "\n";
        std::cout << "Frontier window     : processed=" << res.processed_seeds
                  << ", next_cursor=" << res.next_offset << "\n";
        std::cout << "Nodes visited       : " << res.nodes_visited << "\n";
        std::cout << "Terminal positions  : " << res.terminal_positions << "\n";
        std::cout << "Max depth reached   : " << res.max_depth_reached << "\n";
        std::cout << "Resume token        : " << res.resume_token << "\n";
        std::cout << "=========================\n";
        return 0;
    }

    return smoke_test(argc, argv);
}