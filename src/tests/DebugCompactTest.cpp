#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include "Traversal.h"

TEST_CASE("Debug compact checkpoint reconstruction", "[debug][compact]") {
    // Start with just a few expansions for easier debugging
    Traversal t1;
    t1.start_root_only(Game());

    // Do just a couple expansions
    for (int i = 0; i < 5 && t1.step_one(); ++i) {}

    REQUIRE(t1.pending_frames() > 0);

    const std::string compact_path = "debug_compact.bin";
    REQUIRE(t1.save_checkpoint_compact(compact_path));

    // Load it back
    Traversal t2;
    REQUIRE(t2.load_checkpoint_compact(compact_path));
    REQUIRE(t2.pending_frames() == t1.pending_frames());

    // Debug: Check if the first frame can step successfully
    INFO("Original traversal pending frames: " << t1.pending_frames());
    INFO("Loaded traversal pending frames: " << t2.pending_frames());

    // Take just one step from each
    bool step1 = t1.step_one();
    bool step2 = t2.step_one();

    INFO("Original step result: " << step1);
    INFO("Loaded step result: " << step2);

    REQUIRE(step1 == step2);

    // Clean up
    std::filesystem::remove(compact_path);
}
