#include "Traversal.h"
#include "Game.h"
#include "Piece.h"
#include <chrono>
#include <cstddef>
#include <optional>

namespace {
// Interval for emitting progress events.
constexpr std::chrono::milliseconds kProgressInterval{2000}; // 2 seconds
} // namespace

// Helper function to emit progress if 2 seconds have elapsed
void Traversal::emit_progress_if_needed() {
    if (!progress_cb_) { return; }

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_progress_time_);

    if (elapsed >= kProgressInterval) { // NOLINT(readability-magic-numbers) - documented constant
        ProgressEvent const progress_event{.games = game_count};
        progress_cb_(progress_event);
        last_progress_time_ = now;
    }
}

void Traversal::traverse_impl(Game& game, std::size_t depth) { // NOLINT(misc-no-recursion) recursive by design
    // Check timeout first
    if (deadline_ && std::chrono::steady_clock::now() >= *deadline_) {
        return; // Time's up, stop traversal
    }

    // Check for early termination conditions.
    const auto& move_count = game.move_count();
    const auto& is_looping = game.is_looping();
    if (move_count == 0 || is_looping) {
        // Game is over - emit result
        ++game_count;
        const auto winner = game.player() == PieceColor::BLACK ? PieceColor::WHITE : PieceColor::BLACK;
        ResultEvent const result_event{
            .game_id = game_count,
            .looping = is_looping,
            .winner = is_looping ? std::nullopt : std::make_optional(winner),
            .history = game.get_move_sequence(),
        };
        // Invoke result callback
        if (result_cb_) { result_cb_(result_event); }

        // Emit progress every 2 seconds
        emit_progress_if_needed();

        // No need to undo here - backtracking will handle it
        return;
    }

    // Record a checkpoint entry for this decision depth (progress index, maximum choices)
    checkpoint_.push_back(CheckpointEntry{0u, static_cast<std::size_t>(move_count)});
    // Snapshot the non-empty checkpoint stack so we can expose a final
    // meaningful checkpoint even if traversal empties the stack later.
    last_checkpoint_snapshot_ = checkpoint_;

    // Iterate choices using the checkpoint's progress_index so the current path is visible
    while (checkpoint_.back().progress_index < checkpoint_.back().maximum_index) {
        const std::size_t idx = checkpoint_.back().progress_index;
        game.select_move(idx);
        traverse_impl(game, depth + 1); // Pass incremented depth
        game.undo_move();               // Backtrack after exploring this branch

        // Advance the recorded progress for this depth
        ++checkpoint_.back().progress_index;
        // If deadline reached during recursion, stop exploring further siblings
        if (deadline_ && std::chrono::steady_clock::now() >= *deadline_) {
            checkpoint_.pop_back();
            return;
        }
    }

    // Leaving this decision point - remove the checkpoint entry
    checkpoint_.pop_back();
    // Update snapshot if there are still entries, otherwise keep the last known good state
    if (!checkpoint_.empty()) { last_checkpoint_snapshot_ = checkpoint_; }
}

void Traversal::traverse_for(Game& game, std::optional<std::chrono::milliseconds> timeout) {
    // Initialize
    game_count = 0;
    last_progress_time_ = std::chrono::steady_clock::now();

    // Clear any previous checkpoint state before starting
    checkpoint_.clear();

    // Set deadline if timeout is provided
    if (timeout) {
        deadline_ = std::chrono::steady_clock::now() + *timeout;
    } else {
        deadline_ = std::nullopt;
    }

    traverse_impl(game);

    // If traversal finished and we didn't leave a checkpoint on the stack,
    // but we captured a last non-empty snapshot earlier, restore it so
    // external callers can inspect a meaningful final checkpoint.
    if (checkpoint_.empty() && !last_checkpoint_snapshot_.empty()) { checkpoint_ = last_checkpoint_snapshot_; }
    // If we still have no checkpoint, provide a placeholder so callers see
    // a deterministic final state instead of an empty vector.
    if (checkpoint_.empty() && last_checkpoint_snapshot_.empty()) { checkpoint_.push_back(CheckpointEntry{0u, 0u}); }

    // traversal end
}
