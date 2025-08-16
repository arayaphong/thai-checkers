#include "Traversal.h"
#include <chrono>

// Helper function to emit progress if 2 seconds have elapsed
void Traversal::emit_progress_if_needed() {
    if (!progress_cb_) return;

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_progress_time_);

    if (elapsed >= std::chrono::milliseconds(2000)) { // 2 seconds
        ProgressEvent prog_ev{.games = game_count};
        progress_cb_(prog_ev);
        last_progress_time_ = now;
    }
}

void Traversal::traverse_impl(Game& game, std::size_t depth) {
    // Check timeout first
    if (deadline_ && std::chrono::steady_clock::now() >= *deadline_) {
        return; // Time's up, stop traversal
    }

    // // Add depth limit to prevent infinite exploration
    // const std::size_t MAX_DEPTH = 50; // Reasonable limit for demonstration
    // if (depth >= MAX_DEPTH) {
    //     // Treat as draw due to depth limit
    //     ++game_count;
    //     ResultEvent ev{
    //         .game_id = game_count,
    //         .looping = false,
    //         .winner = std::nullopt, // Draw due to depth limit
    //         .history = game.get_move_sequence(),
    //     };
    //     if (result_cb_) result_cb_(ev);

    //     // Emit progress every 2 seconds
    //     emit_progress_if_needed();
    //     return;
    // }

    // Check for early termination conditions.
    const std::size_t move_count = game.move_count();
    if (move_count == 0) {
        // Game is over - emit result
        ++game_count;
        const auto is_looping = game.is_looping();
        const auto winner = game.player() == PieceColor::BLACK ? PieceColor::WHITE : PieceColor::BLACK;
        ResultEvent ev{
            .game_id = game_count,
            .looping = is_looping,
            .winner = is_looping ? std::nullopt : std::make_optional(winner),
            .history = game.get_move_sequence(),
        };
        // Invoke result callback
        if (result_cb_) result_cb_(ev);

        // Emit progress every 2 seconds
        emit_progress_if_needed();

        // No need to undo here - backtracking will handle it
        return;
    }

    for (std::size_t i = 0; i < move_count; ++i) {
        game.select_move(i);
        traverse_impl(game, depth + 1); // Pass incremented depth
        game.undo_move();               // Backtrack after exploring this branch
    }
}

void Traversal::traverse_for(Game& game, std::optional<std::chrono::milliseconds> timeout) {
    // Initialize
    game_count = 0;
    last_progress_time_ = std::chrono::steady_clock::now();

    // Set deadline if timeout is provided
    if (timeout) {
        deadline_ = std::chrono::steady_clock::now() + *timeout;
    } else {
        deadline_ = std::nullopt;
    }

    traverse_impl(game);
}
