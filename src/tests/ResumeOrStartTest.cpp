#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include "Traversal.h"

namespace {
struct Collector {
    std::vector<Traversal::ResultEvent> evs;
};
} // namespace

TEST_CASE("resume_or_start reproduces same early results as manual load+stepping", "[checkpoint][resume]") {
    // Prepare a partial checkpoint
    Traversal prep;
    prep.start_root_only(Game());
    const int kPrepExpansions = 35; // small
    int done = 0;
    while (done < kPrepExpansions && prep.step_one()) ++done;
    REQUIRE(prep.pending_frames() > 0);
    const std::string fname = "checkpoint_resume.bin";
    REQUIRE(prep.save_checkpoint(fname));

    // Manual path: load then step collecting first K results
    constexpr std::size_t kTargetGames = 3;
    Traversal manual;
    Collector A;
    manual.set_result_callback([&](const Traversal::ResultEvent& ev) {
        if (A.evs.size() < kTargetGames) A.evs.push_back(ev);
    });
    REQUIRE(manual.load_checkpoint(fname));
    std::size_t safety_steps = 0;
    const std::size_t kMaxSteps = 200000; // safety
    while (A.evs.size() < kTargetGames && safety_steps < kMaxSteps && manual.step_one()) ++safety_steps;
    REQUIRE(!A.evs.empty());

    // resume_or_start path with stop injected after K results
    Traversal resumed;
    Collector B;
    resumed.set_result_callback([&](const Traversal::ResultEvent& ev) {
        if (B.evs.size() < kTargetGames) B.evs.push_back(ev);
        if (B.evs.size() >= kTargetGames) resumed.request_stop();
    });
    resumed.resume_or_start(fname); // should load and then run_from_work_stack, stopping via callback

    REQUIRE(B.evs.size() == A.evs.size());
    for (std::size_t i = 0; i < A.evs.size(); ++i) {
        REQUIRE(A.evs[i].move_history == B.evs[i].move_history);
        REQUIRE(A.evs[i].looping == B.evs[i].looping);
        REQUIRE(A.evs[i].history == B.evs[i].history);
    }

    std::filesystem::remove(fname);
}
