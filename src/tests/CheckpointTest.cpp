#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <set>
#include "Traversal.h"

namespace {
struct Collected {
    std::vector<Traversal::ResultEvent> events;
};
} // namespace

TEST_CASE("Checkpoint roundtrip mid-search produces identical first few terminal games", "[checkpoint]") {
    // Root-only quick roundtrip first (trivial stack)
    {
        Traversal t;
        t.start_root_only(Game());
        const std::string f = "checkpoint_root.bin";
        REQUIRE(t.save_checkpoint(f));
        Traversal t2;
        REQUIRE(t2.load_checkpoint(f));
        std::filesystem::remove(f);
    }

    Traversal tr;
    Collected original;
    tr.set_result_callback([&](const Traversal::ResultEvent& ev) { original.events.push_back(ev); });
    tr.start_root_only(Game());

    // Build a bit of stack without finishing many games (small number of expansions)
    int expansions = 0;
    const int kExpansionsBefore = 40; // keep small to avoid long runtime
    while (expansions < kExpansionsBefore && tr.step_one()) { ++expansions; }
    REQUIRE(tr.pending_frames() > 0); // we should have some work left

    const std::string fname = "checkpoint_mid.bin";
    REQUIRE(tr.save_checkpoint(fname));

    Traversal tr2;
    Collected resumed;
    tr2.set_result_callback([&](const Traversal::ResultEvent& ev) { resumed.events.push_back(ev); });
    REQUIRE(tr2.load_checkpoint(fname));

    // Step both traversals in lockstep until we collect a small number of terminal games or hit limits
    const std::size_t kTargetGames = 2;
    const std::size_t kMaxSteps = 200000; // safety cap
    std::size_t steps = 0;
    while (steps < kMaxSteps && (original.events.size() < kTargetGames || resumed.events.size() < kTargetGames)) {
        bool a = tr.step_one();
        bool b = tr2.step_one();
        if (!a && !b) break; // both finished
        ++steps;
    }

    // Ensure we either reached target or both exhausted identically
    REQUIRE(original.events.size() == resumed.events.size());
    REQUIRE(original.events.size() <= kTargetGames); // we didn't overshoot target unexpectedly
    for (std::size_t i = 0; i < original.events.size(); ++i) {
        REQUIRE(original.events[i].move_history == resumed.events[i].move_history);
        REQUIRE(original.events[i].looping == resumed.events[i].looping);
        REQUIRE(original.events[i].history == resumed.events[i].history);
    }
    std::filesystem::remove(fname);
}
