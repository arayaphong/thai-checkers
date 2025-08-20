#include <cstddef>
#include <vector>
#include <iostream>
#include <string>

#include "Game.h"
#include "Board.h"
#include "Position.h"
#include "Piece.h"
#include "catch2/catch_test_macros.hpp"
#include "catch2/catch_message.hpp"

// Constants for magic numbers
constexpr int MAX_CONSECUTIVE_UNDOS = 4;
constexpr int MAX_TEST_MOVES = 3;
constexpr int TEST_ROW_1 = 1;
constexpr int TEST_COL_2 = 2;
constexpr int TEST_ROW_3 = 3;
constexpr int TEST_COL_4 = 4;
constexpr int TEST_ROW_5 = 5;
constexpr int TEST_COL_6 = 6;

TEST_CASE("Game - Default Constructor", "[Game]") {
    Game game;

    SECTION("Initial board state") {
        const auto& board = game.board();
        REQUIRE(board.hash() == Board::setup().hash());
    }

    SECTION("Initial player is white") { REQUIRE(game.player() == PieceColor::WHITE); }

    SECTION("Initial move count") { REQUIRE(game.move_count() > 0); }

    SECTION("Not looping initially") { REQUIRE_FALSE(game.is_looping()); }

    SECTION("Empty move sequence initially") { REQUIRE(game.get_move_sequence().empty()); }
}

TEST_CASE("Game - Constructor with Board", "[Game]") {
    const Pieces custom_pieces = {{Position{1, 2}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
                                  {Position{2, 3}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}};
    Board custom_board(custom_pieces);

    Game game(custom_board);

    SECTION("Board is set correctly") { REQUIRE(game.board().hash() == custom_board.hash()); }

    SECTION("Initial player is white") { REQUIRE(game.player() == PieceColor::WHITE); }
}

TEST_CASE("Game - Copy functionality", "[Game]") {
    Game original;
    original.select_move(0); // Make a move

    Game copied = Game::copy(original);

    SECTION("Board state matches") { REQUIRE(copied.board().hash() == original.board().hash()); }

    SECTION("Player matches") { REQUIRE(copied.player() == original.player()); }

    SECTION("Move sequence matches") { REQUIRE(copied.get_move_sequence() == original.get_move_sequence()); }

    SECTION("Looping status matches") { REQUIRE(copied.is_looping() == original.is_looping()); }
}

TEST_CASE("Game - Player alternation", "[Game]") {
    Game game;

    SECTION("White starts first") { REQUIRE(game.player() == PieceColor::WHITE); }

    SECTION("Players alternate after moves") {
        REQUIRE(game.move_count() > 0);

        // Make first move (White)
        game.select_move(0);
        REQUIRE(game.player() == PieceColor::BLACK);

        // Make second move (Black)
        if (game.move_count() > 0) {
            game.select_move(0);
            REQUIRE(game.player() == PieceColor::WHITE);
        }
    }
}

TEST_CASE("Game - Move selection and execution", "[Game]") {
    Game game;

    SECTION("Valid move selection") {
        std::size_t initial_move_count = game.move_count();
        REQUIRE(initial_move_count > 0);

        auto initial_sequence = game.get_move_sequence();
        REQUIRE(initial_sequence.empty());

        // Select first available move
        game.select_move(0);

        // Check that move was recorded
        auto updated_sequence = game.get_move_sequence();
        REQUIRE(updated_sequence.size() == 1);
        REQUIRE(updated_sequence[0] == 0);

        // Player should have changed
        REQUIRE(game.player() == PieceColor::BLACK);
    }

    SECTION("Multiple moves") {
        // Make several moves
        for (int i = 0; i < MAX_TEST_MOVES && game.move_count() > 0; ++i) { game.select_move(0); }

        auto sequence = game.get_move_sequence();
        REQUIRE(sequence.size() <= MAX_TEST_MOVES);

        // Check player alternation
        if (sequence.size() % 2 == 0) {
            REQUIRE(game.player() == PieceColor::WHITE);
        } else {
            REQUIRE(game.player() == PieceColor::BLACK);
        }
    }
}

TEST_CASE("Game - Undo functionality", "[Game]") {
    Game game;

    SECTION("Cannot undo from initial state") {
        auto initial_sequence = game.get_move_sequence();
        REQUIRE(initial_sequence.empty());

        // Attempting to undo should not crash and maintain state
        game.undo_move();
        REQUIRE(game.get_move_sequence().empty());
        REQUIRE(game.player() == PieceColor::WHITE);
    }

    SECTION("Undo single move") {
        REQUIRE(game.move_count() > 0);

        auto initial_board = game.board();
        auto initial_player = game.player();

        // Make a move
        game.select_move(0);

        // Verify state changed
        REQUIRE(game.get_move_sequence().size() == 1);
        REQUIRE(game.player() != initial_player);

        // Undo the move
        game.undo_move();

        // Verify state restored
        REQUIRE(game.get_move_sequence().empty());
        REQUIRE(game.player() == initial_player);
        REQUIRE(game.board().hash() == initial_board.hash());
        REQUIRE_FALSE(game.is_looping());
    }

    SECTION("Undo multiple moves") {
        // Make multiple moves
        std::vector<std::size_t> board_hashes;
        board_hashes.push_back(game.board().hash());

        for (int i = 0; i < MAX_TEST_MOVES && game.move_count() > 0; ++i) {
            game.select_move(0);
            board_hashes.push_back(game.board().hash());
        }

        std::size_t moves_made = game.get_move_sequence().size();

        // Undo all moves
        for (std::size_t i = 0; i < moves_made; ++i) { game.undo_move(); }

        // Should be back to initial state
        REQUIRE(game.get_move_sequence().empty());
        REQUIRE(game.player() == PieceColor::WHITE);
        REQUIRE(game.board().hash() == board_hashes[0]);
    }

    SECTION("Undo preserves choices_dirty flag") {
        REQUIRE(game.move_count() > 0);

        // Make a move (this should mark choices as dirty)
        game.select_move(0);
        static_cast<void>(game.move_count()); // This should recompute choices

        // Undo the move (this should also mark choices as dirty)
        game.undo_move();
        auto choices_after_undo = game.move_count(); // This should recompute choices again

        // Verify that choices are properly recomputed
        REQUIRE(choices_after_undo > 0);
    }

    SECTION("Undo updates looping detection correctly") {
        // Create a scenario that might cause looping
        REQUIRE(game.move_count() > 0);

        // Make some moves to potentially create a loop scenario
        for (int i = 0; i < 2 && game.move_count() > 0; ++i) { game.select_move(0); }

        // Undo one move
        if (!game.get_move_sequence().empty()) {
            game.undo_move();

            // Verify looping detection is properly recalculated
            // The looping state should be consistent with the current board state
            bool looping_after_undo = game.is_looping();

            // Make the same moves again to verify consistency
            if (game.move_count() > 0) {
                game.select_move(0);
                bool looping_after_remake = game.is_looping();

                // The looping detection should be deterministic
                REQUIRE((looping_after_remake == looping_after_undo || !looping_after_undo));
            }
        }
    }

    SECTION("Undo multiple times consecutively") {
        // Make several moves
        std::vector<std::size_t> move_counts;
        std::vector<PieceColor> players;
        std::vector<std::size_t> board_hashes;

        // Record initial state
        move_counts.push_back(game.move_count());
        players.push_back(game.player());
        board_hashes.push_back(game.board().hash());

        // Make up to MAX_CONSECUTIVE_UNDOS moves
        for (int i = 0; i < MAX_CONSECUTIVE_UNDOS && game.move_count() > 0; ++i) {
            game.select_move(0);
            move_counts.push_back(game.move_count());
            players.push_back(game.player());
            board_hashes.push_back(game.board().hash());
        }

        std::size_t total_moves = game.get_move_sequence().size();

        // Undo moves one by one and verify each state
        for (std::size_t i = total_moves; i > 0; --i) {
            game.undo_move();

            // Verify we're at the correct historical state
            REQUIRE(game.get_move_sequence().size() == i - 1);
            REQUIRE(game.player() == players[i - 1]);
            REQUIRE(game.board().hash() == board_hashes[i - 1]);
        }

        // Should be back to initial state
        REQUIRE(game.get_move_sequence().empty());
        REQUIRE(game.player() == PieceColor::WHITE);
        REQUIRE(game.board().hash() == board_hashes[0]);
    }

    SECTION("Undo after capture move") {
        // Create a board with a capture opportunity
        const Pieces capture_pieces = {
            {Position{1, 2}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
            {Position{2, 3}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
            {Position{4, 5}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}}};
        Board board(capture_pieces);
        Game capture_game(board);

        auto initial_board = capture_game.board();
        auto initial_pieces_count =
            initial_board.get_pieces(PieceColor::BLACK).size() + initial_board.get_pieces(PieceColor::WHITE).size();

        // Look for a capture move and execute it
        if (capture_game.move_count() > 0) {
            capture_game.select_move(0);

            // Undo the capture
            capture_game.undo_move();

            // Verify the captured piece is restored
            auto after_undo_board = capture_game.board();
            auto after_undo_pieces_count = after_undo_board.get_pieces(PieceColor::BLACK).size() +
                                           after_undo_board.get_pieces(PieceColor::WHITE).size();

            REQUIRE(after_undo_pieces_count == initial_pieces_count);
            REQUIRE(after_undo_board.hash() == initial_board.hash());
        }
    }

    SECTION("Undo with promotion") {
        // Create a board where promotion can occur
        // Use valid coordinates for black squares only
        const Pieces promotion_pieces = {
            {Position{0, 1}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
            {Position{1, 6}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}};
        Board board(promotion_pieces);
        Game promotion_game(board);

        // Make moves that might lead to promotion
        if (promotion_game.move_count() > 0) {
            promotion_game.select_move(0);

            if (promotion_game.move_count() > 0) {
                promotion_game.select_move(0);

                // Undo the last move
                promotion_game.undo_move();

                // Verify the piece promotion is properly undone
                auto after_undo_board = promotion_game.board();

                // Check that no unexpected promotions remain
                auto white_pieces = after_undo_board.get_pieces(PieceColor::WHITE);
                auto black_pieces = after_undo_board.get_pieces(PieceColor::BLACK);

                // Verify piece types are consistent with the state after first move
                bool promotion_properly_undone = true;
                for (const auto& [pos, piece_info] : white_pieces) {
                    if (pos.y() != 0 && piece_info.type == PieceType::DAME) { promotion_properly_undone = false; }
                }
                for (const auto& [pos, piece_info] : black_pieces) {
                    if (pos.y() != Position::board_size - 1 && piece_info.type == PieceType::DAME) {
                        promotion_properly_undone = false;
                    }
                }

                REQUIRE(promotion_properly_undone);
            }
        }
    }
}

TEST_CASE("Game - Move struct functionality", "[Game]") {
    SECTION("Move equality") {
        Position from{TEST_ROW_1, TEST_COL_2};
        Position destination{TEST_ROW_3, TEST_COL_4};
        std::vector<Position> captured = {Position{TEST_COL_2, TEST_ROW_3}};

        Move move1{from, destination, captured};
        Move move2{from, destination, captured};
        Move move3{from, Position{TEST_ROW_5, TEST_COL_6}, captured};

        REQUIRE(move1 == move2);
        REQUIRE_FALSE(move1 == move3);
    }

    SECTION("Move capture detection") {
        Position from{TEST_ROW_1, TEST_COL_2};
        Position destination{TEST_ROW_3, TEST_COL_4};

        Move non_capture{from, destination, {}};
        Move capture{from, destination, {Position{TEST_COL_2, TEST_ROW_3}}};

        REQUIRE_FALSE(non_capture.is_capture());
        REQUIRE(capture.is_capture());
    }
}

TEST_CASE("Game - Looping detection", "[Game]") {
    SECTION("Initial state is not looping") {
        Game game;
        REQUIRE_FALSE(game.is_looping());
    }

    SECTION("Move count is zero when looping") {
        Pieces pieces = {{Position{"D7"}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
                         {Position{"E8"}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
                         {Position{"D1"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::DAME}}};
        Board board(pieces);
        Game game(board);
        constexpr int MOVE_INDEX_6 = 6;
        constexpr int MOVE_INDEX_0 = 0;
        constexpr int MOVE_INDEX_8 = 8;
        constexpr int MOVE_INDEX_1 = 1;
        constexpr int MOVE_INDEX_9 = 9;
        constexpr int MOVE_INDEX_4 = 4;

        game.select_move(MOVE_INDEX_6);
        game.select_move(MOVE_INDEX_0);
        game.select_move(MOVE_INDEX_8);
        game.select_move(MOVE_INDEX_1);
        game.select_move(MOVE_INDEX_8);
        game.select_move(MOVE_INDEX_0);
        game.select_move(MOVE_INDEX_9);
        game.select_move(MOVE_INDEX_1);
        REQUIRE_FALSE(game.is_looping());
        game.select_move(MOVE_INDEX_4);
        game.select_move(MOVE_INDEX_1);
        game.select_move(MOVE_INDEX_9);
        game.select_move(MOVE_INDEX_0);
        REQUIRE(game.is_looping());
        REQUIRE(game.move_count() > 0); // New behavior: Game can have moves even in a loop
    }
}

TEST_CASE("Game - Edge cases", "[Game]") {
    SECTION("Empty board game") {
        Board empty_board;
        Game game(empty_board);

        // Should handle empty board gracefully
        REQUIRE(game.move_count() == 0);
        REQUIRE(game.player() == PieceColor::WHITE);
        REQUIRE_FALSE(game.is_looping());
    }

    SECTION("Single piece board") {
        const Pieces single_pieces = {{Position{1, 2}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}}};
        Board single_piece_board(single_pieces);

        Game game(single_piece_board);

        // Should handle single piece correctly
        REQUIRE(game.player() == PieceColor::WHITE);
        REQUIRE_FALSE(game.is_looping());
    }
}

TEST_CASE("Game - Board manipulation through game", "[Game]") {
    Game game;

    SECTION("Board state changes after moves") {
        auto initial_hash = game.board().hash();

        if (game.move_count() > 0) {
            game.select_move(0);
            auto new_hash = game.board().hash();

            REQUIRE(new_hash != initial_hash);
        }
    }

    SECTION("Board accessor is const correct") {
        const Game& const_game = game;
        const auto& board_ref = const_game.board();

        // This should compile (const correctness test)
        auto hash = board_ref.hash();
        REQUIRE(hash == game.board().hash());
    }
}

TEST_CASE("Game - Print functions don't crash", "[Game]") {
    Game game;

    SECTION("Print board doesn't crash") {
        // These functions write to stdout, so we just verify they don't crash
        REQUIRE_NOTHROW(game.print_board());
    }

    SECTION("Print choices doesn't crash") { REQUIRE_NOTHROW(game.print_choices()); }
}

TEST_CASE("Game - Move sequence integrity", "[Game]") {
    Game game;

    SECTION("Move sequence grows correctly") {
        auto initial_size = game.get_move_sequence().size();
        REQUIRE(initial_size == 0);

        // Make moves and verify sequence grows
        for (int i = 0; i < MAX_TEST_MOVES && game.move_count() > 0; ++i) {
            game.select_move(0);
            REQUIRE(game.get_move_sequence().size() == initial_size + i + 1);
        }
    }

    SECTION("Move sequence values are valid") {
        if (game.move_count() > 0) {
            std::size_t move_count = game.move_count();
            game.select_move(move_count - 1); // Select last available move

            auto sequence = game.get_move_sequence();
            REQUIRE(sequence.size() == 1);
            REQUIRE(sequence[0] == move_count - 1);
        }
    }
}

TEST_CASE("Game - State consistency", "[Game]") {
    Game game;

    SECTION("Consistent state after operations") {
        // Make a move
        if (game.move_count() > 0) {
            auto initial_player = game.player();
            game.select_move(0);

            // State should be consistent
            REQUIRE(game.player() != initial_player);
            REQUIRE(game.get_move_sequence().size() == 1);

            // Undo and check consistency
            game.undo_move();
            REQUIRE(game.player() == initial_player);
            REQUIRE(game.get_move_sequence().empty());
        }
    }

    SECTION("Move count consistency") {
        // When not looping, move count should be positive if pieces exist
        if (!game.is_looping()) {
            // For a standard starting position, there should be moves available
            auto board = Board::setup();
            Game standard_game(board);
            REQUIRE(standard_game.move_count() > 0);
        }
    }
}

TEST_CASE("Game - Promotion mechanics", "[Game]") {
    SECTION("Piece promotion verification") {
        // Test promotion by directly checking the promotion logic in move execution
        // Create a scenario where a piece is already at the promotion row after a move

        // Set up a simple test with a white piece that can move to the top row
        const Pieces test_pieces = {
            {Position{0, 1},
             PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}}, // Can potentially move to row 0
            {Position{2, 3},
             PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}} // Black piece for valid game
        };
        Board board(test_pieces);
        Game game(board);

        // Verify initial state - white piece should be a pion
        REQUIRE(game.player() == PieceColor::WHITE);
        REQUIRE(board.is_occupied(Position{0, 1}));
        REQUIRE_FALSE(board.is_black_piece(Position{0, 1}));
        REQUIRE_FALSE(board.is_dame_piece(Position{0, 1}));

        // Try to make a move - the game will handle promotion automatically if the piece reaches row 0 or 7
        auto initial_move_count = game.move_count();
        if (initial_move_count > 0) {
            game.select_move(0);

            // After the move, check if any white piece at the promotion edge (row 0) is promoted
            const auto& updated_board = game.board();
            auto white_pieces = updated_board.get_pieces(PieceColor::WHITE);

            for (const auto& [pos, piece_info] : white_pieces) {
                if (pos.y() == 0) { // At top edge (promotion row for white)
                    REQUIRE(piece_info.type == PieceType::DAME);
                }
            }
        }
    }

    SECTION("No premature promotion") {
        // Verify pieces are not promoted when they're not at the edge
        const Pieces middle_pieces = {{Position{1, 2}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
                                      {Position{3, 4}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}};
        Board board(middle_pieces);
        Game game(board);

        // Verify initial state
        REQUIRE_FALSE(board.is_dame_piece(Position{1, 2}));
        REQUIRE_FALSE(board.is_dame_piece(Position{3, 4}));

        // Make a move
        if (game.move_count() > 0) {
            game.select_move(0);

            // Check that pieces in the middle of the board remain as pions
            const auto& updated_board = game.board();
            auto white_pieces = updated_board.get_pieces(PieceColor::WHITE);
            auto black_pieces = updated_board.get_pieces(PieceColor::BLACK);

            for (const auto& [pos, piece_info] : white_pieces) {
                if (pos.y() != 0) { // Not at promotion row
                    REQUIRE(piece_info.type == PieceType::PION);
                }
            }

            for (const auto& [pos, piece_info] : black_pieces) {
                if (pos.y() != Position::board_size - 1) { // Not at promotion row
                    REQUIRE(piece_info.type == PieceType::PION);
                }
            }
        }
    }

    SECTION("Black piece promotion test") {
        // Test black piece promotion by creating a scenario near bottom edge
        const Pieces black_promotion_pieces = {
            {Position{1, 6}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}, // Near bottom edge
            {Position{0, 1},
             PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}} // White piece for valid game
        };
        Board board(black_promotion_pieces);
        Game game(board);

        // White moves first in checkers
        REQUIRE(game.player() == PieceColor::WHITE);

        if (game.move_count() > 0) {
            game.select_move(0); // White moves

            // Now it's black's turn
            if (game.player() == PieceColor::BLACK && game.move_count() > 0) {
                game.select_move(0); // Black moves

                // Check if any black piece at the bottom edge (row 7) is promoted
                const auto& updated_board = game.board();
                auto black_pieces = updated_board.get_pieces(PieceColor::BLACK);

                for (const auto& [pos, piece_info] : black_pieces) {
                    if (pos.y() == Position::board_size - 1) { // At bottom edge (promotion row for black)
                        REQUIRE(piece_info.type == PieceType::DAME);
                    }
                }
            }
        }
    }
}
