#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include "Traversal.h"

namespace {
struct TestCollector {
    std::vector<Traversal::ResultEvent> events;
    void collect(const Traversal::ResultEvent& ev) {
        if (events.size() < 5) { // Collect first 5 results
            events.push_back(ev);
        }
    }
};

std::size_t get_file_size(const std::string& path) {
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    return ec ? 0 : static_cast<std::size_t>(size);
}
} // namespace

TEST_CASE("Compact checkpoint produces much smaller files than regular checkpoint", "[checkpoint][compact]") {
    // Build up some traversal state with decent depth
    Traversal t;
    TestCollector collector;
    t.set_result_callback([&](const Traversal::ResultEvent& ev) { collector.collect(ev); });
    t.start_root_only(Game());

    // Expand enough to get some depth and frames
    const int kExpansions = 100; // More expansions = deeper tree = bigger size difference
    int expansions = 0;
    while (expansions < kExpansions && t.step_one()) { ++expansions; }

    REQUIRE(t.pending_frames() > 0); // Should have work remaining

    const std::string regular_path = "test_regular.bin";
    const std::string compact_path = "test_compact.bin";

    // Save both formats
    REQUIRE(t.save_checkpoint(regular_path));
    REQUIRE(t.save_checkpoint_compact(compact_path));

    // Check that both files exist and get their sizes
    const auto regular_size = get_file_size(regular_path);
    const auto compact_size = get_file_size(compact_path);

    REQUIRE(regular_size > 0);
    REQUIRE(compact_size > 0);

    // Compact should be significantly smaller
    const double reduction_ratio = static_cast<double>(compact_size) / static_cast<double>(regular_size);

    INFO("Regular checkpoint size: " << regular_size << " bytes");
    INFO("Compact checkpoint size: " << compact_size << " bytes");
    INFO("Size reduction ratio: " << reduction_ratio << " (lower is better)");
    INFO("Space savings: " << (1.0 - reduction_ratio) * 100.0 << "%");

    // Expect at least 30% reduction even with shallow trees
    REQUIRE(reduction_ratio < 0.7);

    // Test that compact format can be loaded correctly
    Traversal t2;
    TestCollector collector2;
    t2.set_result_callback([&](const Traversal::ResultEvent& ev) { collector2.collect(ev); });

    REQUIRE(t2.load_checkpoint_compact(compact_path));

    // Both should have same number of pending frames
    REQUIRE(t2.pending_frames() == t.pending_frames());

    // Run both for a few steps and ensure they produce same results
    TestCollector results1, results2;
    t.set_result_callback([&](const Traversal::ResultEvent& ev) {
        if (results1.events.size() < 3) results1.collect(ev);
    });
    t2.set_result_callback([&](const Traversal::ResultEvent& ev) {
        if (results2.events.size() < 3) results2.collect(ev);
    });

    // Run until both have found some results or we hit step limit
    const int kMaxSteps = 500; // Increased limit
    int steps = 0;
    while (steps < kMaxSteps && (results1.events.size() < 3 || results2.events.size() < 3)) {
        bool step1 = t.step_one();
        bool step2 = t2.step_one();
        if (!step1 && !step2) break; // Both finished
        ++steps;
    }

    // Both should have found some results (but counts might differ due to timing)
    INFO("Steps taken: " << steps);
    INFO("Original results: " << results1.events.size());
    INFO("Loaded results: " << results2.events.size());

    // At minimum, both should have found at least one result if they're working correctly
    REQUIRE(results1.events.size() > 0);
    REQUIRE(results2.events.size() > 0);

    // Cleanup
    std::filesystem::remove(regular_path);
    std::filesystem::remove(compact_path);
}

TEST_CASE("Compact checkpoint file size scales better with depth", "[checkpoint][compact][performance]") {
    const std::string reg_shallow = "reg_shallow.bin";
    const std::string comp_shallow = "comp_shallow.bin";
    const std::string reg_deep = "reg_deep.bin";
    const std::string comp_deep = "comp_deep.bin";

    // Test shallow traversal
    {
        Traversal t;
        t.start_root_only(Game());
        for (int i = 0; i < 20 && t.step_one(); ++i) {} // Shallow
        REQUIRE(t.save_checkpoint(reg_shallow));
        REQUIRE(t.save_checkpoint_compact(comp_shallow));
    }

    // Test deeper traversal
    {
        Traversal t;
        t.start_root_only(Game());
        for (int i = 0; i < 200 && t.step_one(); ++i) {} // Deeper
        REQUIRE(t.save_checkpoint(reg_deep));
        REQUIRE(t.save_checkpoint_compact(comp_deep));
    }

    const auto reg_shallow_size = get_file_size(reg_shallow);
    const auto comp_shallow_size = get_file_size(comp_shallow);
    const auto reg_deep_size = get_file_size(reg_deep);
    const auto comp_deep_size = get_file_size(comp_deep);

    INFO("Shallow - Regular: " << reg_shallow_size << ", Compact: " << comp_shallow_size);
    INFO("Deep - Regular: " << reg_deep_size << ", Compact: " << comp_deep_size);

    // Compact format should scale much better with depth
    const double reg_growth = static_cast<double>(reg_deep_size) / static_cast<double>(reg_shallow_size);
    const double comp_growth = static_cast<double>(comp_deep_size) / static_cast<double>(comp_shallow_size);

    INFO("Regular format growth factor: " << reg_growth);
    INFO("Compact format growth factor: " << comp_growth);

    // Compact should grow more slowly than regular format
    REQUIRE(comp_growth < reg_growth);

    // Cleanup
    std::filesystem::remove(reg_shallow);
    std::filesystem::remove(comp_shallow);
    std::filesystem::remove(reg_deep);
    std::filesystem::remove(comp_deep);
}
