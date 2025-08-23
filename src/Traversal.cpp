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

void Traversal::traverse_impl(Game& game) { // Iterative traversal using explicit stack
    // Initialize with the root state
    const auto move_count = game.move_count();
    const auto is_looping = game.is_looping();

    // Handle immediate termination (no moves or looping at start)
    if (move_count == 0 || is_looping) {
        ++game_count;
        const auto winner = game.player() == PieceColor::BLACK ? PieceColor::WHITE : PieceColor::BLACK;
        ResultEvent const result_event{
            .game_id = game_count,
            .looping = is_looping,
            .winner = is_looping ? std::nullopt : std::make_optional(winner),
            .history = game.get_move_sequence(),
        };
        if (result_cb_) { result_cb_(result_event); }
        emit_progress_if_needed();
        return;
    }

    // Start with initial checkpoint
    checkpoint_.push_back(CheckpointEntry{0u, static_cast<std::size_t>(move_count)});

    // Main iterative loop - process the checkpoint stack
    while (!checkpoint_.empty()) {
        // Check timeout
        if (deadline_ && std::chrono::steady_clock::now() >= *deadline_) {
            break; // Time's up, stop traversal
        }

        auto& current_checkpoint = checkpoint_.back();

        // If we've explored all moves at this level, backtrack
        if (current_checkpoint.progress_index >= current_checkpoint.maximum_index) {
            checkpoint_.pop_back();
            if (!checkpoint_.empty()) {
                // Undo the move that brought us to this level
                game.undo_move();
                // Advance progress at the parent level
                ++checkpoint_.back().progress_index;
            }
            continue;
        }

        // Select the next move at current level
        const std::size_t idx = current_checkpoint.progress_index;
        game.select_move(idx);

        // Check the new game state after the move
        const auto new_move_count = game.move_count();
        const auto new_is_looping = game.is_looping();

        if (new_move_count == 0 || new_is_looping) {
            // Game is over - emit result
            ++game_count;
            const auto winner = game.player() == PieceColor::BLACK ? PieceColor::WHITE : PieceColor::BLACK;
            ResultEvent const result_event{
                .game_id = game_count,
                .looping = new_is_looping,
                .winner = new_is_looping ? std::nullopt : std::make_optional(winner),
                .history = game.get_move_sequence(),
            };
            if (result_cb_) { result_cb_(result_event); }
            emit_progress_if_needed();

            // Backtrack immediately - no need to go deeper
            game.undo_move();
            ++current_checkpoint.progress_index;
        } else {
            // Game continues - go deeper by adding new checkpoint
            checkpoint_.push_back(CheckpointEntry{0u, static_cast<std::size_t>(new_move_count)});
        }
    }
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

    // If we finished with an empty checkpoint (complete traversal),
    // provide a placeholder so callers see a deterministic final state.
    if (checkpoint_.empty()) { checkpoint_.push_back(CheckpointEntry{0u, 0u}); }

    // traversal end
}
