#include "Game.h"
#include "Traversal.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <format>
#include <cstring>
#include <csignal>
#include <thread>

namespace {
volatile std::sig_atomic_t g_stop_flag = 0;
extern "C" void on_stop_signal(int) { g_stop_flag = 1; }
} // namespace

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    Traversal traversal;
    // Subscribe to Traversal events only; no local aggregation
    traversal.set_result_callback([&](const Traversal::ResultEvent& /*ev*/) {
        // Build score tree here
    });
    traversal.set_summary_callback([&](const Traversal::SummaryEvent& s) {
        std::cout << std::format("\n===== Callback Summary =====\n");
        std::cout << std::format("Wall time: {:.3f}s\n", s.wall_seconds);
        std::cout << std::format("Games: {}\n", s.games);
        std::cout << std::format("Throughput: {:.2f} games/s\n", s.throughput_games_per_sec);
        if (s.cpu_seconds >= 0.0) {
            std::cout << std::format("CPU time: {:.3f}s\n", s.cpu_seconds);
            if (s.cpu_util_percent >= 0.0) { std::cout << std::format("CPU util: {:.1f}%\n", s.cpu_util_percent); }
        }
        if (s.rss_kb > 0) std::cout << std::format("RSS: {} MB\n", s.rss_kb / 1024);
        if (s.hwm_kb > 0) std::cout << std::format("Peak RSS: {} MB\n", s.hwm_kb / 1024);
    });

    traversal.set_progress_interval(std::chrono::milliseconds(3000));
    traversal.set_progress_callback(
        [&](const Traversal::ProgressEvent& p) { std::cout << std::format("[progress] games so far: {}\n", p.games); });

    long ms = 10000; // default to 10s if not set
    if (const char* ms_env = std::getenv("TRAVERSAL_MS"); ms_env && *ms_env) {
        // Special value for infinite traversal
        if (std::strcmp(ms_env, "INF") == 0 || std::strcmp(ms_env, "inf") == 0) {
            // Install cooperative stop on SIGINT/SIGTERM
            std::signal(SIGINT, on_stop_signal);
            std::signal(SIGTERM, on_stop_signal);
            std::thread stopper([&]() {
                while (!g_stop_flag) std::this_thread::sleep_for(std::chrono::milliseconds(25));
                traversal.request_stop();
            });
            traversal.traverse();
            g_stop_flag = 1;
            if (stopper.joinable()) stopper.join();
            return 0;
        }
        char* end = nullptr;
        const long parsed = std::strtol(ms_env, &end, 10);
        if (end != ms_env && parsed > 0) ms = parsed;
        // else keep default 10s
    } else {
        // Expose default in the environment for visibility to any downstream tools
        setenv("TRAVERSAL_MS", "10000", 0);
    }
    traversal.traverse_for(std::chrono::milliseconds(ms));
    return 0;
}
