#include "Game.h"
#include "Traversal.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <format>

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
    Traversal traversal;
    // Subscribe to Traversal events only; no local aggregation
    traversal.set_result_callback([&](const Traversal::ResultEvent& /*ev*/) {
        // No-op in this demo. You can inspect ev.total_games, ev.winner/looping, ev.move_history here.
    });
    traversal.set_summary_callback([&](const Traversal::SummaryEvent& s) {
        std::cout << std::format("\n===== Callback Summary =====\n");
        std::cout << std::format("Wall time: {:.3f}s\n", s.wall_seconds);
        std::cout << std::format("Games: {}\n", s.games);
        std::cout << std::format("Throughput: {:.2f} games/s\n", s.throughput_games_per_sec);
        if (s.cpu_seconds >= 0.0) {
            std::cout << std::format("CPU time: {:.3f}s\n", s.cpu_seconds);
            if (s.cpu_util_percent >= 0.0) {
                std::cout << std::format("CPU util: {:.1f}%\n", s.cpu_util_percent);
            }
        }
    if (s.rss_kb > 0) std::cout << std::format("RSS: {} MB\n", s.rss_kb / 1024);
    if (s.hwm_kb > 0) std::cout << std::format("Peak RSS: {} MB\n", s.hwm_kb / 1024);
    });

    traversal.set_progress_interval(std::chrono::milliseconds(3000));
    traversal.set_progress_callback([&](const Traversal::ProgressEvent& p) {
        std::cout << std::format("[progress] games so far: {}\n", p.games);
    });

    if (const char* ms_env = std::getenv("TRAVERSAL_MS"); ms_env) {
        const long ms = std::strtol(ms_env, nullptr, 10);
        if (ms > 0) {
            traversal.traverse_for(std::chrono::milliseconds(ms));
            return 0;
        }
    }
    traversal.traverse();
}