#include "Game.h"
#include "Traversal.h"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <format>
#include <cstring>
#include <filesystem>

namespace {
// Default checkpoint path
const std::string CHECKPOINT_PATH = "thai_checkers_checkpoint.compact";
} // namespace

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    Traversal traversal;

    // Performance optimizations for maximum speed
    traversal.set_high_performance_mode(true);
    traversal.set_memory_vs_speed_ratio(0.8); // 80% speed, 20% memory consideration
    traversal.set_task_depth_limit(6);
    traversal.set_loop_detection_aggressive(false);

    // Check for command line arguments
    bool resume_mode = false;
    bool clean_start = false;
    std::string checkpoint_path = CHECKPOINT_PATH;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--resume") == 0 || std::strcmp(argv[i], "-r") == 0) {
            resume_mode = true;
        } else if (std::strcmp(argv[i], "--clean") == 0 || std::strcmp(argv[i], "-c") == 0) {
            clean_start = true;
        } else if (std::strcmp(argv[i], "--checkpoint") == 0 && i + 1 < argc) {
            checkpoint_path = argv[++i];
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            std::cout << "Thai Checkers High-Performance Traversal\n\n";
            std::cout << "Usage: " << argv[0] << " [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  -r, --resume           Resume from checkpoint if available\n";
            std::cout << "  -c, --clean            Start clean (remove existing checkpoint)\n";
            std::cout << "  --checkpoint <path>    Use custom checkpoint file path\n";
            std::cout << "  -h, --help             Show this help message\n\n";
            std::cout << "Environment variables:\n";
            std::cout << "  TRAVERSAL_MS=<ms>      Duration in milliseconds (default: 10000)\n\n";
            return 0;
        }
    }

    // Clean start: remove existing checkpoint
    if (clean_start && std::filesystem::exists(checkpoint_path)) {
        std::filesystem::remove(checkpoint_path);
        std::cout << "Removed existing checkpoint: " << checkpoint_path << "\n";
    }

    // Check if checkpoint exists for resume
    const bool has_checkpoint = std::filesystem::exists(checkpoint_path);
    if (resume_mode && !has_checkpoint) {
        std::cout << "No checkpoint found at: " << checkpoint_path << "\n";
        std::cout << "Starting fresh traversal...\n";
        resume_mode = false;
    } else if (has_checkpoint && !resume_mode) {
        std::cout << "Checkpoint found: " << checkpoint_path << "\n";
        std::cout << "Use --resume to continue from checkpoint, or --clean to start fresh\n";
    }

    // High-performance optimizations
    traversal.set_high_performance_mode(true);
    traversal.set_memory_vs_speed_ratio(0.8);       // Favor speed over memory
    traversal.set_task_depth_limit(6);              // Deeper parallelization
    traversal.set_loop_detection_aggressive(false); // Skip some loop checks for speed

    // Subscribe to Traversal events only; no local aggregation
    traversal.set_result_callback([&](const Traversal::ResultEvent& ev) {
        // TODO: Build score tree here
        const std::string winner_str = ev.winner.has_value() ? std::to_string(static_cast<int>(*ev.winner)) : "none";
        std::string winner_label;
        if (!ev.winner) {
            winner_label = "Draw";
        } else {
            switch (static_cast<int>(*ev.winner)) {
            case 0:
                winner_label = "Black";
                break;
            case 1:
                winner_label = "White";
                break;
            default:
                winner_label = std::format("unknown({})", static_cast<int>(*ev.winner));
                break;
            }
        }

        std::cout << std::format("[result] game_id: {}, winner: {}, moves: {}\n", ev.game_id, winner_label,
                                 ev.history.size());
    });

    // Enhanced summary callback with checkpoint info
    traversal.set_summary_callback([&](const Traversal::SummaryEvent& s) {
        // Always show as Current Session Summary since checkpoint resume summary is skipped
        std::cout << std::format("\n===== Current Session Summary =====\n");
        std::cout << std::format("Wall time: {:.3f}s\n", s.wall_seconds);
        std::cout << std::format("Previous games: {}\n", s.previous_games);
        std::cout << std::format("Current games: {}\n", s.games);
        std::cout << std::format("Total games: {}\n", s.total_games);
        std::cout << std::format("Throughput: {:.2f} games/s\n", s.throughput_games_per_sec);
        if (s.cpu_seconds >= 0.0) {
            std::cout << std::format("CPU time: {:.3f}s\n", s.cpu_seconds);
            if (s.cpu_util_percent >= 0.0) { std::cout << std::format("CPU util: {:.1f}%\n", s.cpu_util_percent); }
        }
        if (s.rss_kb > 0) std::cout << std::format("RSS: {} MB\n", s.rss_kb / 1024);
        if (s.hwm_kb > 0) std::cout << std::format("Peak RSS: {} MB\n", s.hwm_kb / 1024);

        // Save checkpoint on completion
        std::cout << "Saving checkpoint... ";
        if (traversal.save_checkpoint_compact(checkpoint_path, true)) {
            const auto size = std::filesystem::file_size(checkpoint_path);
            std::cout << std::format("saved {} KB\n", size / 1024);
        } else {
            std::cout << "failed\n";
        }
    });

    traversal.set_progress_interval(std::chrono::milliseconds(5000)); // Less frequent updates for better performance
    traversal.set_progress_callback([&](const Traversal::ProgressEvent& p) {
        std::cout << std::format("[progress] games: {}\n", p.games);

        // Auto-save checkpoint every progress interval for safety
        if (traversal.save_checkpoint_compact(checkpoint_path + ".tmp", true)) {
            std::filesystem::rename(checkpoint_path + ".tmp", checkpoint_path);
        }
    });

    long ms = 10000; // default to 10s if not set
    if (const char* ms_env = std::getenv("TRAVERSAL_MS"); ms_env && *ms_env) {
        char* end = nullptr;
        const long parsed = std::strtol(ms_env, &end, 10);
        if (end != ms_env && parsed > 0) ms = parsed;
        // else keep default 10s
    } else {
        // Expose default in the environment for visibility to any downstream tools
        setenv("TRAVERSAL_MS", "10000", 0);
    }

    // Timed traversal with checkpoint support
    if (resume_mode && has_checkpoint) {
        std::cout << std::format("Resuming from checkpoint: {} ({}ms duration)\n", checkpoint_path, ms);
        traversal.resume_or_start(checkpoint_path, Game());
        traversal.traverse_for_continue(std::chrono::milliseconds(ms), Game());
    } else {
        std::cout << std::format("Starting fresh traversal ({}ms duration)\n", ms);
        traversal.traverse_for(std::chrono::milliseconds(ms));
    }

    // Save final checkpoint
    std::cout << "Final checkpoint save... ";
    if (traversal.save_checkpoint_compact(checkpoint_path, true)) {
        const auto size = std::filesystem::file_size(checkpoint_path);
        std::cout << std::format("saved {} KB\n", size / 1024);
    } else {
        std::cout << "failed\n";
    }

    return 0;
}
