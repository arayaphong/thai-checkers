// Minimal runner for simplified Traversal
#include "Game.h"
#include "Traversal.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <format>
#include <limits>
#include <string>
#include <string_view>
#include <optional>
#include <fstream>

namespace {
constexpr long long k_milliseconds_per_second = 1000;
constexpr long long k_default_timeout_ms = 10000;
} // namespace

auto parse_timeout(std::string_view arg) -> std::optional<std::chrono::milliseconds> {
    if (arg.empty()) { return std::nullopt; }

    try {
        // Check for 'ms' suffix (milliseconds) FIRST - before checking 's'
        if (arg.ends_with("ms")) {
            const auto ms_str = arg.substr(0, arg.length() - 2);
            const long long milliseconds = std::stoll(std::string(ms_str));
            return std::chrono::milliseconds(milliseconds);
        }

        // Check for 's' suffix (seconds)
        if (arg.ends_with('s')) {
            const auto seconds_str = arg.substr(0, arg.length() - 1);
            const double seconds = std::stod(std::string(seconds_str));
            return std::chrono::milliseconds(static_cast<long long>(seconds * k_milliseconds_per_second));
        }

        // Default to seconds if no suffix
        const double seconds = std::stod(std::string(arg));
        return std::chrono::milliseconds(static_cast<long long>(seconds * k_milliseconds_per_second));
    } catch (const std::exception&) { return std::nullopt; }
}

void print_usage(const char* program_name) {
    std::cout << std::format("Usage: {} [--timeout DURATION]\n", program_name);
    std::cout << "Options:\n";
    std::cout << "  --timeout DURATION  Set timeout duration (e.g., 10s, 12.5s, 5000ms)\n";
    std::cout << "                      Default: 10s\n";
    std::cout << "  --help             Show this help message\n";
}

// Save checkpoint as a compact JSON object to a file. Returns true on success.
static bool save_checkpoint_json_to_file(const std::vector<Traversal::CheckpointEntry>& cp,
                                         const std::string& filename) {
    std::ofstream ofs(filename, std::ios::trunc);
    if (!ofs) return false;

    ofs << '{';
    ofs << "\"checkpoint\":";
    ofs << '[';
    for (std::size_t i = 0; i < cp.size(); ++i) {
        const auto& e = cp[i];
        ofs << '{';
        ofs << "\"progress\":" << e.progress_index << ',';
        ofs << "\"maximum\":" << e.maximum_index;
        ofs << '}';
        if (i + 1 < cp.size()) ofs << ',';
    }
    ofs << ']';
    ofs << '}';
    ofs << '\n';
    return true;
}

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

    // Save checkpoint as JSON before exit
    const std::string cp_file = "checkpoint.json";
    const auto checkpoint = traversal.get_checkpoint();

    if (save_checkpoint_json_to_file(checkpoint, cp_file)) {
        std::cout << std::format("Checkpoint saved: {} depth to {}\n", checkpoint.size(), cp_file);
    } else {
        std::cerr << std::format("Error: failed to save checkpoint to {}\n", cp_file);
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
