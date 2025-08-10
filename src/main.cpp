#include <chrono>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
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
    std::mt19937 g_rng{std::random_device{}()};
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
            g_rng.seed(static_cast<std::mt19937::result_type>(seed));
        }
    }
    const auto wall_start = std::chrono::steady_clock::now();
    const auto usage_start = capture_usage();
    std::size_t games_played = 0;

    if (g_quiet)
    {
        while (std::chrono::steady_clock::now() - wall_start < test_duration)
        {
            random_game_play_quiet();
            ++games_played;
        }
    }
    else
    {
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
    return smoke_test(argc, argv);
}