#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include "Traversal.h"

TEST_CASE("Debug compact checkpoint callback issue", "[debug][callback]") {
    // Create original traversal with some depth
    Traversal original;
    std::vector<Traversal::ResultEvent> original_results;
    original.set_result_callback([&](const Traversal::ResultEvent& ev) {
        original_results.push_back(ev);
        INFO("Original callback triggered for game " << ev.game_id);
    });

    original.start_root_only(Game());

    // Expand to get some frames
    int expansions = 0;
    while (expansions < 100 && original.step_one()) { ++expansions; }

    INFO("Original traversal: " << original.pending_frames() << " pending frames");
    INFO("Original results so far: " << original_results.size());

    // Continue until we get at least 2 terminal results
    while (original_results.size() < 2 && original.step_one()) {
        // Keep stepping until we have results
    }

    REQUIRE(original_results.size() >= 2);
    INFO("Original found " << original_results.size() << " terminal games");

    // Save compact checkpoint at this point
    const std::string compact_path = "debug_callback.bin";
    REQUIRE(original.save_checkpoint_compact(compact_path));

    // Load into new traversal
    Traversal loaded;
    std::vector<Traversal::ResultEvent> loaded_results;
    loaded.set_result_callback([&](const Traversal::ResultEvent& ev) {
        loaded_results.push_back(ev);
        INFO("Loaded callback triggered for game " << ev.game_id);
    });

    REQUIRE(loaded.load_checkpoint_compact(compact_path));
    INFO("Loaded traversal: " << loaded.pending_frames() << " pending frames");

    // Try to get the same number of results
    int steps = 0;
    while (loaded_results.size() < 2 && steps < 1000 && loaded.step_one()) { ++steps; }

    INFO("Loaded results after " << steps << " steps: " << loaded_results.size());

    // If no results, debug further
    if (loaded_results.empty()) {
        INFO("No results from loaded traversal - checking individual steps");

        // Try a few individual steps with detailed logging
        for (int i = 0; i < 10 && loaded.pending_frames() > 0; ++i) {
            auto frames_before = loaded.pending_frames();
            bool stepped = loaded.step_one();
            auto frames_after = loaded.pending_frames();

            INFO("Step " << i << ": " << frames_before << " -> " << frames_after << " frames, stepped=" << stepped
                         << ", results=" << loaded_results.size());

            if (!stepped) break;
        }
    }

    // For now, just verify the checkpoint mechanism works and callbacks function
    REQUIRE(loaded_results.size() > 0);   // Should have found some results
    REQUIRE(loaded.pending_frames() > 0); // Should still have work to do

    std::filesystem::remove(compact_path);
}
