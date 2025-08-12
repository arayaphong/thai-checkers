#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include "Traversal.h"

TEST_CASE("Compact checkpoint file size validation", "[compact][size]") {
    // Create a deeper traversal to emphasize the benefits
    Traversal t;
    t.start_root_only(Game());

    // Build significant depth
    const int kExpansions = 200;
    int expansions = 0;
    while (expansions < kExpansions && t.step_one()) { ++expansions; }

    REQUIRE(t.pending_frames() > 0);

    const std::string regular_path = "size_test_regular.bin";
    const std::string compact_path = "size_test_compact.bin";

    // Save both formats
    REQUIRE(t.save_checkpoint(regular_path));
    REQUIRE(t.save_checkpoint_compact(compact_path));

    // Get file sizes
    const auto regular_size = std::filesystem::file_size(regular_path);
    const auto compact_size = std::filesystem::file_size(compact_path);

    INFO("Regular checkpoint: " << regular_size << " bytes");
    INFO("Compact checkpoint: " << compact_size << " bytes");

    const double reduction = 1.0 - (static_cast<double>(compact_size) / static_cast<double>(regular_size));
    INFO("Space savings: " << (reduction * 100.0) << "%");

    // Expect significant reduction
    REQUIRE(compact_size < regular_size);
    REQUIRE(reduction > 0.5); // At least 50% reduction

    // Test that both can be loaded and have same state
    Traversal t_reg, t_comp;
    REQUIRE(t_reg.load_checkpoint(regular_path));
    REQUIRE(t_comp.load_checkpoint_compact(compact_path));
    REQUIRE(t_reg.pending_frames() == t_comp.pending_frames());

    // Test equivalence by running a few steps
    for (int i = 0; i < 10; ++i) {
        bool reg_step = t_reg.step_one();
        bool comp_step = t_comp.step_one();
        REQUIRE(reg_step == comp_step);
        if (!reg_step) break; // Both finished
    }

    // Cleanup
    std::filesystem::remove(regular_path);
    std::filesystem::remove(compact_path);
}
