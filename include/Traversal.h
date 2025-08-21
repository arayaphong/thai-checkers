#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <unordered_set>
#include <vector>
#include "Game.h"

class Traversal {
  public:
    // Checkpoint entry exposed publicly so callers can inspect the traversal state.
    struct CheckpointEntry {
        std::size_t progress_index;
        std::size_t maximum_index;
    };

    // Return a copy of the current checkpoint stack (depth-ordered, root -> current).
    // If the live checkpoint is empty, return the last non-empty snapshot so
    // callers always receive a meaningful final checkpoint.
    std::vector<CheckpointEntry> get_checkpoint() const {
        return !checkpoint_.empty() ? checkpoint_ : last_checkpoint_snapshot_;
    }

    void traverse_for(Game& game, std::optional<std::chrono::milliseconds> timeout = std::nullopt);
    struct ResultEvent {
        std::size_t game_id;              // total games completed so far
        bool looping;                     // true if ended due to repetition
        std::optional<PieceColor> winner; // winner if not looping
        std::vector<uint8_t> history;     // sequence of board state hashes (initial + each resulting state)
    };
    struct ProgressEvent {
        std::size_t games;
    };
    // Construct with optional callbacks
    explicit Traversal(std::function<void(const ResultEvent&)> result_cb = {},
                       std::function<void(const ProgressEvent&)> progress_cb = {})
        : result_cb_(std::move(result_cb)), progress_cb_(std::move(progress_cb)) {}

  private:
    // Counter for completed games (single-threaded)
    std::size_t game_count{0};

    // Timeout deadline
    std::optional<std::chrono::steady_clock::time_point> deadline_;

    // Last time progress was emitted
    std::chrono::steady_clock::time_point last_progress_time_{std::chrono::steady_clock::now()};

    // Depth-ordered checkpoint snapshot (root -> current). Traversal code should
    // push/pop or update entries as it descends/backtracks. This is intentionally
    // a lightweight structure (no full history) for resuming/inspecting current path.
    std::vector<CheckpointEntry> checkpoint_;

    // Last non-empty checkpoint observed during traversal. Used to restore a
    // meaningful checkpoint when the traversal unwinds completely.
    std::vector<CheckpointEntry> last_checkpoint_snapshot_;

    // Depth-aware traversal to limit task creation overhead
    void traverse_impl(Game& game, std::size_t depth = 0);

    // Helper to emit progress every 2 seconds
    void emit_progress_if_needed();

    // Subscribers
    const std::function<void(const ResultEvent&)> result_cb_;
    const std::function<void(const ProgressEvent&)> progress_cb_;
};
