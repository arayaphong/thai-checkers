// Minimal runner for simplified Traversal
#include "Game.h"
#include "Traversal.h"
#include "utils.h"
#include <chrono>
#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {
constexpr long long k_milliseconds_per_second = 1000;
constexpr long long k_default_timeout_ms = 10000;
} // namespace

auto main(int argc, const char* const* argv) -> int {
    // Default timeout: 10 seconds
    std::chrono::milliseconds timeout{k_default_timeout_ms};

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
        if (arg == "--timeout") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --timeout requires a duration argument\n";
                print_usage(argv[0]);
                return 1;
            }

            const auto parsed_timeout = parse_timeout(argv[++i]);
            if (!parsed_timeout) {
                std::cerr << std::format("Error: Invalid timeout format '{}'\n", argv[i]);
                std::cerr << "Expected format: 10s, 12.5s, or 5000ms\n";
                return 1;
            }

            timeout = *parsed_timeout;
        } else {
            std::cerr << std::format("Error: Unknown argument '{}'\n", arg);
            print_usage(argv[0]);
            return 1;
        }
    }

    std::cout << std::format("Running Thai Checkers analysis with timeout: {}ms\n", timeout.count());

    // Record start time for runtime calculation
    const auto start_time = std::chrono::steady_clock::now();

    // Game statistics counters
    uint64_t loops = 0;
    uint64_t black = 0;
    uint64_t white = 0;
    uint64_t min_moves = std::numeric_limits<uint64_t>::max();
    uint64_t max_moves = 0;

    // Subscribe to result events: update statistics and capture checkpoint.
    Traversal traversal(
        [&](const Traversal::ResultEvent& result_event) {
            const auto moves = static_cast<uint64_t>(result_event.history.size());
            if (!result_event.winner) {
                ++loops;
            } else if (static_cast<int>(*result_event.winner) == 0) {
                ++black;
            } else {
                ++white;
            }
            min_moves = std::min(min_moves, moves);
            max_moves = std::max(max_moves, moves);
        },
        [&](const Traversal::ProgressEvent& progress_event) {
            std::cout << std::format("Progress: {} games completed\n", progress_event.games);
        });

    Game game;
    traversal.traverse_for(game, timeout);

    // Calculate actual runtime
    const auto end_time = std::chrono::steady_clock::now();
    const auto runtime_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    const double runtime_seconds =
        static_cast<double>(runtime_duration.count()) / static_cast<double>(k_milliseconds_per_second);

    // Save checkpoint as JSON before exit
    const std::string cp_file = "checkpoint_" + std::to_string(black + white + loops) + ".log";
    const auto checkpoint = traversal.get_checkpoint();

    if (save_checkpoint_to_file(checkpoint, cp_file)) {
        const std::string completed_percentage = calculate_completion_percentage(checkpoint);
        const double throughput =
            static_cast<double>(black + white + loops) /
            (static_cast<double>(timeout.count()) / static_cast<double>(k_milliseconds_per_second));

        // Append completion information to checkpoint file with full precision
        std::ofstream ofs(cp_file, std::ios::app);
        if (ofs) {
            ofs << std::format("# Depth: {}\n", checkpoint.size());
            ofs << std::format("# Completion (range 0.0 - 1.0): {}\n", completed_percentage);
            ofs << std::format("# Throughput: {:.3f} games/s\n", throughput);
            ofs << std::format("# Runtime: {:.3f} seconds\n", runtime_seconds);
        }

        std::cout << std::format("Checkpoint saved to '{}'\n", cp_file);
    }

    // Print game statistics
    std::cout << std::format("Game statistics:\n");
    std::cout << std::format("  Loops: {}\n", loops);
    std::cout << std::format("  Black wins: {}\n", black);
    std::cout << std::format("  White wins: {}\n", white);
    std::cout << std::format("  Min moves: {}\n", min_moves);
    std::cout << std::format("  Max moves: {}\n", max_moves);
    std::cout << std::format("  Total games: {}\n", black + white + loops);
    std::cout << std::format("  Throughput: {:.3f} games/s\n", static_cast<double>(black + white + loops) /
                                                                   (static_cast<double>(timeout.count()) /
                                                                    static_cast<double>(k_milliseconds_per_second)));

    return 0;
}
