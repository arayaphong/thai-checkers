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
#include "Traversal.h"

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

// Removed in-file traversal; now provided by Traversal class.

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
    Traversal::Options topts{};

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
        // Capture profiling start
        const auto wall_start = std::chrono::steady_clock::now();
        const auto usage_start = capture_usage();

        // Provide sane defaults if user gave no bounds to avoid long runs
        if (topts.max_frontier == 0 && topts.time_limit.count() == 0 && topts.node_limit == 0 && topts.leaf_limit == 0)
        {
            topts.max_frontier = 100; // process at most 100 frontier seeds per run by default
        }
            // If only time limit is specified, still set a modest frontier window to enable parallelism
            if (topts.max_frontier == 0 && topts.time_limit.count() > 0)
            {
    #ifdef _OPENMP
                topts.max_frontier = 4 * (omp_get_max_threads() > 0 ? omp_get_max_threads() : 1);
    #else
                topts.max_frontier = 4;
    #endif
            }
        const auto res = Traversal::run(topts);

        // Capture profiling end
        const auto wall_end = std::chrono::steady_clock::now();
        const auto usage_end = capture_usage();

        std::cout << "\n==== Traverse Summary ====\n";
        std::cout << "Split depth         : " << topts.split_depth << "\n";
        std::cout << "Frontier window     : processed=" << res.processed_seeds
                  << ", next_cursor=" << res.next_offset << "\n";
        std::cout << "Nodes visited       : " << res.nodes_visited << "\n";
        std::cout << "Terminal positions  : " << res.terminal_positions << "\n";
        std::cout << "Max depth reached   : " << res.max_depth_reached << "\n";
        std::cout << "Resume token        : " << res.resume_token << "\n";
        std::cout << "=========================\n";

        // Print a profiling summary similar to smoke_test
        const auto wall_seconds = std::chrono::duration<double>(wall_end - wall_start).count();
        const double cpu_user = usage_end.user_seconds - usage_start.user_seconds;
        const double cpu_system = usage_end.system_seconds - usage_start.system_seconds;
        const double cpu_total = cpu_user + cpu_system;
        const double cpu_util_percent = wall_seconds > 0.0 ? (cpu_total / wall_seconds) * 100.0 : 0.0;

        std::cout << "\n==== Profiling Summary ====\n";
        std::cout << "Wall time           : " << wall_seconds << " s\n";
        // Provide a couple of relevant throughput stats for traversal
        if (wall_seconds > 0.0)
        {
            const double nodes_per_second = static_cast<double>(res.nodes_visited) / wall_seconds;
            const double seeds_per_second = static_cast<double>(res.processed_seeds) / wall_seconds;
            const double games_per_second = static_cast<double>(res.terminal_positions) / wall_seconds;
            std::cout << "Seeds processed     : " << res.processed_seeds << " (" << seeds_per_second << "/s)\n";
            std::cout << "Nodes visited rate  : " << nodes_per_second << " /s\n";
            std::cout << "Games / second      : " << games_per_second << "\n";
        }
        std::cout << "CPU user time       : " << cpu_user << " s\n";
        std::cout << "CPU system time     : " << cpu_system << " s\n";
        std::cout << "CPU total time      : " << cpu_total << " s\n";
        std::cout << "CPU utilization     : " << cpu_util_percent << " % (total CPU time / wall)\n";
        std::cout << "Current RSS         : " << usage_end.current_rss_kb << " KB\n";
        std::cout << "Max RSS (ru_maxrss) : " << usage_end.max_rss_kb << " KB\n";
        std::cout << "===========================\n";
        return 0;
    }

    return smoke_test(argc, argv);
}