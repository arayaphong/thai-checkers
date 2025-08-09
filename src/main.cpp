#include "Game.h"
#include "main.h"
#include <iostream>
#include <Options.h>
#include <random>
#include <chrono>
#include <sys/resource.h>
#include <unistd.h>
#include <fstream>

namespace {
struct usage_snapshot {
    double user_seconds{0.0};
    double system_seconds{0.0};
    long max_rss_kb{0};
    size_t current_rss_kb{0};
};

usage_snapshot capture_usage() {
    usage_snapshot snap;
    rusage ru{};
    if (getrusage(RUSAGE_SELF, &ru) == 0) {
        snap.user_seconds = static_cast<double>(ru.ru_utime.tv_sec) + ru.ru_utime.tv_usec / 1'000'000.0;
        snap.system_seconds = static_cast<double>(ru.ru_stime.tv_sec) + ru.ru_stime.tv_usec / 1'000'000.0;
        // On Linux ru_maxrss is in kilobytes
        snap.max_rss_kb = ru.ru_maxrss;
    }
    // Get current resident set size (in KB)
    std::ifstream statm("/proc/self/statm");
    if (statm) {
        size_t size_pages = 0, resident_pages = 0;
        statm >> size_pages >> resident_pages; // we only need the first two
        long page_kb = sysconf(_SC_PAGESIZE) / 1024;
        snap.current_rss_kb = resident_pages * static_cast<size_t>(page_kb);
    }
    return snap;
}
} // namespace

void print_choices(const std::vector<Move>& choices) {
    for (const auto& m : choices) {
        std::cout << "From: " << m.from.to_string() << " To: " << m.to.to_string();
        if (!m.captured.empty()) {
            std::cout << " (Captures: ";
            for (std::size_t i = 0; i < m.captured.size(); ++i) {
                std::cout << m.captured[i].to_string();
                if (i + 1 < m.captured.size()) std::cout << ", ";
            }
            std::cout << ")";
        }
        std::cout << '\n';
    }
}

void random_game_play()
{
    Game game;
    int state = 0;
    
    while (true) {
        const auto& current_player = (state % 2 == 0) ? "White" : "Black";
        std::cout << "[State " << state << "] " << current_player << " Turn:" << std::endl;
        game.print_board();
        std::cout << std::endl;

        const auto& choices = game.get_choices();
        if (choices.empty()) {
            std::cout << "No more moves available. Game over!" << std::endl;
            std::string winner = (state % 2 == 0) ? "Black" : "White";
            std::cout << "Victory: " << winner << " wins!" << std::endl;
            break;
        }

        print_choices(choices);
        std::cout << std::endl;

        // Randomly select an available move for demonstration
        static std::mt19937 rng(std::random_device{}());               // good-quality PRNG, seeded once
        std::uniform_int_distribution<std::size_t> dist(0, choices.size() - 1);
        const auto& move = choices[dist(rng)];
        std::cout << "Selected move: " << move.from.to_string() << " -> " << move.to.to_string() << std::endl;
        game.execute_move(move);
        
        ++state;
    }
}

int main() {
    using namespace std::chrono;
    constexpr auto test_duration = seconds{10};
    const auto wall_start = steady_clock::now();
    const auto usage_start = capture_usage();
    std::size_t games_played = 0;

    while (steady_clock::now() - wall_start < test_duration) {
        random_game_play();
        ++games_played;
    }

    const auto wall_end = steady_clock::now();
    const auto usage_end = capture_usage();

    const auto wall_seconds = duration<double>(wall_end - wall_start).count();
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
    std::cout << "Note: Extensive per-move printing will heavily reduce games/second.\n";

    return 0;
}