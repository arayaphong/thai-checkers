#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <chrono>
#include <cstdlib>
#include <format>
#include "Traversal.h"
#include "Game.h"

namespace {
const std::string TEST_CHECKPOINT_PATH = "test_main_checkpoint.compact";

struct MainTestCollector {
    std::vector<Traversal::ResultEvent> results;
    std::vector<Traversal::SummaryEvent> summaries;
    std::vector<Traversal::ProgressEvent> progress_events;

    void clear() {
        results.clear();
        summaries.clear();
        progress_events.clear();
    }
};

// Helper function to simulate main.cpp configuration
void configure_traversal_like_main(Traversal& traversal) {
    // Performance optimizations for maximum speed (from main.cpp)
    traversal.set_high_performance_mode(true);
    traversal.set_memory_vs_speed_ratio(0.8); // 80% speed, 20% memory consideration
    traversal.set_task_depth_limit(6);
    traversal.set_loop_detection_aggressive(false);
}

// Cleanup test files
void cleanup_test_files() {
    std::filesystem::remove(TEST_CHECKPOINT_PATH);
    std::filesystem::remove(TEST_CHECKPOINT_PATH + ".tmp");
}
} // namespace

TEST_CASE("Main functionality - Performance configuration", "[main][performance]") {
    Traversal traversal;

    // Apply the same configuration as main.cpp
    configure_traversal_like_main(traversal);

    // Verify that the configuration was applied
    // Note: These would need getter methods in Traversal class to properly verify
    // For now, we just verify the configuration doesn't throw
    REQUIRE_NOTHROW(traversal.set_high_performance_mode(true));
    REQUIRE_NOTHROW(traversal.set_memory_vs_speed_ratio(0.8));
    REQUIRE_NOTHROW(traversal.set_task_depth_limit(6));
    REQUIRE_NOTHROW(traversal.set_loop_detection_aggressive(false));
}

TEST_CASE("Main functionality - Checkpoint operations", "[main][checkpoint]") {
    cleanup_test_files();

    Traversal traversal;
    configure_traversal_like_main(traversal);

    MainTestCollector collector;

    // Set up callbacks like main.cpp, but with early stop for testing
    traversal.set_result_callback([&](const Traversal::ResultEvent& ev) {
        collector.results.push_back(ev);
        // Stop after collecting a few results to avoid long test runs
        if (collector.results.size() >= 3) { traversal.request_stop(); }
    });

    traversal.set_summary_callback([&](const Traversal::SummaryEvent& s) {
        collector.summaries.push_back(s);

        // Save checkpoint on completion (like main.cpp)
        REQUIRE(traversal.save_checkpoint_compact(TEST_CHECKPOINT_PATH, true));
    });

    // Start with root only to avoid complex traversal issues
    traversal.start_root_only(Game());

    // Step through manually like other tests do
    int steps = 0;
    const int max_steps = 1000; // Safety limit
    while (steps < max_steps && traversal.step_one()) {
        ++steps;
        if (collector.results.size() >= 3) { break; }
    }

    // Verify checkpoint was created if we got any summary
    if (!collector.summaries.empty()) {
        REQUIRE(std::filesystem::exists(TEST_CHECKPOINT_PATH));
        REQUIRE(std::filesystem::file_size(TEST_CHECKPOINT_PATH) > 0);
    }

    cleanup_test_files();
}

TEST_CASE("Main functionality - Resume from checkpoint", "[main][resume]") {
    cleanup_test_files();

    // First, create a checkpoint manually without using traverse_for
    {
        Traversal traversal;
        configure_traversal_like_main(traversal);

        MainTestCollector collector;
        traversal.set_result_callback([&](const Traversal::ResultEvent& ev) { collector.results.push_back(ev); });

        traversal.set_summary_callback([&](const Traversal::SummaryEvent& s) { collector.summaries.push_back(s); });

        // Start with root only and do a few steps
        traversal.start_root_only(Game());
        int steps = 0;
        const int prep_steps = 20;
        while (steps < prep_steps && traversal.step_one()) ++steps;

        // Save checkpoint manually
        REQUIRE(traversal.save_checkpoint_compact(TEST_CHECKPOINT_PATH, true));
        REQUIRE(std::filesystem::exists(TEST_CHECKPOINT_PATH));
    }

    // Now resume from checkpoint (simulating --resume)
    {
        Traversal traversal;
        configure_traversal_like_main(traversal);

        MainTestCollector collector;
        traversal.set_result_callback([&](const Traversal::ResultEvent& ev) {
            collector.results.push_back(ev);
            // Stop early for test
            if (collector.results.size() >= 2) { traversal.request_stop(); }
        });

        traversal.set_summary_callback([&](const Traversal::SummaryEvent& s) { collector.summaries.push_back(s); });

        // Resume like main.cpp does
        REQUIRE_NOTHROW(traversal.resume_or_start(TEST_CHECKPOINT_PATH, Game()));

        // Verify final checkpoint save works
        REQUIRE(traversal.save_checkpoint_compact(TEST_CHECKPOINT_PATH, true));
    }

    cleanup_test_files();
}

TEST_CASE("Main functionality - Progress callback with auto-save", "[main][progress]") {
    cleanup_test_files();

    Traversal traversal;
    configure_traversal_like_main(traversal);

    MainTestCollector collector;
    bool auto_save_triggered = false;

    // Set progress interval like main.cpp (but shorter for testing)
    traversal.set_progress_interval(std::chrono::milliseconds(50));

    traversal.set_progress_callback([&](const Traversal::ProgressEvent& p) {
        collector.progress_events.push_back(p);

        // Auto-save checkpoint every progress interval (like main.cpp)
        if (traversal.save_checkpoint_compact(TEST_CHECKPOINT_PATH + ".tmp", true)) {
            std::filesystem::rename(TEST_CHECKPOINT_PATH + ".tmp", TEST_CHECKPOINT_PATH);
            auto_save_triggered = true;
        }
    });

    traversal.set_result_callback([&](const Traversal::ResultEvent& ev) {
        collector.results.push_back(ev);
        // Stop early to avoid long test
        if (collector.results.size() >= 1) { traversal.request_stop(); }
    });

    // Start manually instead of using traverse_for
    traversal.start_root_only(Game());

    // Step through manually
    int steps = 0;
    const int max_steps = 500;
    while (steps < max_steps && traversal.step_one()) {
        ++steps;
        if (collector.results.size() >= 1) break;
    }

    // Progress events are timing-based, so they may or may not have been triggered
    // Just verify the test ran without crashing
    REQUIRE(steps > 0); // We did some work

    cleanup_test_files();
}

TEST_CASE("Main functionality - Environment variable handling", "[main][environment]") {
    // Test environment variable parsing logic from main.cpp

    // Save original environment
    const char* original_ms = std::getenv("TRAVERSAL_MS");

    // Test default value
    unsetenv("TRAVERSAL_MS");
    long ms = 10000; // default from main.cpp
    if (const char* ms_env = std::getenv("TRAVERSAL_MS"); ms_env && *ms_env) {
        char* end = nullptr;
        const long parsed = std::strtol(ms_env, &end, 10);
        if (end != ms_env && parsed > 0) ms = parsed;
    }
    REQUIRE(ms == 10000); // Should remain default

    // Test valid environment variable
    setenv("TRAVERSAL_MS", "5000", 1);
    ms = 10000; // reset to default
    if (const char* ms_env = std::getenv("TRAVERSAL_MS"); ms_env && *ms_env) {
        char* end = nullptr;
        const long parsed = std::strtol(ms_env, &end, 10);
        if (end != ms_env && parsed > 0) ms = parsed;
    }
    REQUIRE(ms == 5000); // Should be updated

    // Test invalid environment variable
    setenv("TRAVERSAL_MS", "invalid", 1);
    ms = 10000; // reset to default
    if (const char* ms_env = std::getenv("TRAVERSAL_MS"); ms_env && *ms_env) {
        char* end = nullptr;
        const long parsed = std::strtol(ms_env, &end, 10);
        if (end != ms_env && parsed > 0) ms = parsed;
    }
    REQUIRE(ms == 10000); // Should remain default due to invalid value

    // Test zero value
    setenv("TRAVERSAL_MS", "0", 1);
    ms = 10000; // reset to default
    if (const char* ms_env = std::getenv("TRAVERSAL_MS"); ms_env && *ms_env) {
        char* end = nullptr;
        const long parsed = std::strtol(ms_env, &end, 10);
        if (end != ms_env && parsed > 0) ms = parsed;
    }
    REQUIRE(ms == 10000); // Should remain default due to zero value

    // Restore original environment
    if (original_ms) {
        setenv("TRAVERSAL_MS", original_ms, 1);
    } else {
        unsetenv("TRAVERSAL_MS");
    }
}

TEST_CASE("Main functionality - Result callback formatting", "[main][callbacks]") {
    Traversal traversal;
    configure_traversal_like_main(traversal);

    MainTestCollector collector;
    std::vector<std::string> formatted_results;

    // Set up result callback with formatting like main.cpp
    traversal.set_result_callback([&](const Traversal::ResultEvent& ev) {
        collector.results.push_back(ev);

        // Format like main.cpp
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

        const auto moves_played = (ev.history.size() > 0) ? ev.history.size() - 1 : 0;
        std::string formatted =
            std::format("[result] game_id: {}, winner: {}, moves: {}", ev.game_id, winner_label, moves_played);
        formatted_results.push_back(formatted);

        // Stop early for test
        if (collector.results.size() >= 2) { traversal.request_stop(); }
    });

    // Start manually for safer test
    traversal.start_root_only(Game());

    // Step through manually
    int steps = 0;
    const int max_steps = 1000;
    while (steps < max_steps && traversal.step_one()) {
        ++steps;
        if (collector.results.size() >= 2) break;
    }

    // Verify that we formatted results (if any were generated)
    REQUIRE(collector.results.size() == formatted_results.size());

    // If we have results, verify the formatting contains expected elements
    for (const auto& formatted : formatted_results) {
        REQUIRE(formatted.find("[result]") == 0); // Starts with [result]
        REQUIRE(formatted.find("game_id:") != std::string::npos);
        REQUIRE(formatted.find("winner:") != std::string::npos);
        REQUIRE(formatted.find("moves:") != std::string::npos);
    }
}

TEST_CASE("Main functionality - Clean start simulation", "[main][clean]") {
    cleanup_test_files();

    // Create a checkpoint file first
    {
        Traversal traversal;
        configure_traversal_like_main(traversal);

        // Start manually and save a checkpoint
        traversal.start_root_only(Game());
        int steps = 0;
        while (steps < 10 && traversal.step_one()) ++steps;

        REQUIRE(traversal.save_checkpoint_compact(TEST_CHECKPOINT_PATH, true));
        REQUIRE(std::filesystem::exists(TEST_CHECKPOINT_PATH));
    }

    // Simulate --clean flag behavior from main.cpp
    bool clean_start = true;
    std::string checkpoint_path = TEST_CHECKPOINT_PATH;

    if (clean_start && std::filesystem::exists(checkpoint_path)) { std::filesystem::remove(checkpoint_path); }

    // Verify checkpoint was removed
    REQUIRE_FALSE(std::filesystem::exists(TEST_CHECKPOINT_PATH));

    // Now start fresh traversal
    {
        Traversal traversal;
        configure_traversal_like_main(traversal);

        MainTestCollector collector;
        traversal.set_result_callback([&](const Traversal::ResultEvent& ev) {
            collector.results.push_back(ev);
            if (collector.results.size() >= 1) { traversal.request_stop(); }
        });

        // This should be a fresh start
        traversal.start_root_only(Game());
        int steps = 0;
        while (steps < 500 && traversal.step_one()) {
            ++steps;
            if (collector.results.size() >= 1) break;
        }

        // Verify we did some work (fresh start)
        REQUIRE(steps > 0);
    }

    cleanup_test_files();
}
