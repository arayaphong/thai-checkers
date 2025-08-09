
#include <catch2/catch_all.hpp>
#include "Board.h"
#include "Position.h"

TEST_CASE("Board color-specific piece counting") {
    Position a{1, 0}, b{3, 0}, c{5, 0};
    Pieces pieces = {
        {a, {PieceColor::BLACK, PieceType::PION}},
        {b, {PieceColor::BLACK, PieceType::DAME}},
        {c, {PieceColor::WHITE, PieceType::PION}}
    };
    Board board(pieces);

    REQUIRE(board.get_piece_count(PieceColor::BLACK) == 2);   // Black pieces (PION + DAME)
    REQUIRE(board.get_piece_count(PieceColor::WHITE) == 1);  // White pieces
    REQUIRE(board.get_piece_count() == 3);       // All pieces

    // Remove a black piece and check again
    board.remove_piece(a);
    REQUIRE(board.get_piece_count(PieceColor::BLACK) == 1);
    REQUIRE(board.get_piece_count(PieceColor::WHITE) == 1);
    REQUIRE(board.get_piece_count() == 2);

    // Remove the last black piece
    board.remove_piece(b);
    REQUIRE(board.get_piece_count(PieceColor::BLACK) == 0);
    REQUIRE(board.get_piece_count(PieceColor::WHITE) == 1);
    REQUIRE(board.get_piece_count() == 1);
}

TEST_CASE("Board sequential piece operations") {
    // Create black at a, white at b
    Position a{1, 0};
    Position b{3, 0};
    Position c{0, 1};

    Pieces pieces = {
        {a, {PieceColor::BLACK, PieceType::PION}},
        {b, {PieceColor::WHITE, PieceType::PION}}
    };
    Board board(pieces);

    REQUIRE(board.is_occupied(a));
    REQUIRE(board.is_black_piece(a));
    REQUIRE(board.is_occupied(b));
    REQUIRE(board.get_piece_count() == 2);

    // Move black from a to c
    board.move_piece(a, c);
    REQUIRE(!board.is_occupied(a));
    REQUIRE(board.is_occupied(c));
    REQUIRE(board.is_black_piece(c));

    // Promote black at c
    board.promote_piece(c);
    REQUIRE(board.is_dame_piece(c));

    // Remove white at b
    board.remove_piece(b);
    REQUIRE(!board.is_occupied(b));
    REQUIRE(board.get_piece_count() == 1);
}

TEST_CASE("Board invalid sequences and exception handling") {
    Position a{1, 0};
    Position b{3, 0};
    Position c{0, 1};

    Pieces pieces = {
        {a, {PieceColor::BLACK, PieceType::PION}},
        {b, {PieceColor::WHITE, PieceType::PION}}
    };
    Board board(pieces);

    // Move from empty
    Position empty{5, 0};  // Valid position but empty
    REQUIRE_THROWS(board.move_piece(empty, c));
    // Move to occupied
    REQUIRE_THROWS(board.move_piece(a, b));
    // Promote at empty
    REQUIRE_THROWS(board.promote_piece(empty));
    // Remove at empty
    REQUIRE_NOTHROW(board.remove_piece(empty));
}

TEST_CASE("Board saturation and cycling") {
    Pieces pieces = {};

    int count = 0;
    for (int y = 0; y < 4; ++y) {
        for (int x = (y % 2 == 0 ? 1 : 0); x < 8; x += 2) {
            if (count < 16) {
                pieces[Position{x, y}] = {PieceColor::BLACK, PieceType::PION};
                ++count;
            }
        }
    }
    Board board(pieces);

    REQUIRE(board.get_piece_count() == 16);

    // Remove one, add again
    board.remove_piece(Position{1, 0});
    REQUIRE(board.get_piece_count() == 15);
}

TEST_CASE("Board game-like move chains") {
    Position p1{1, 0}, p2{3, 0}, p3{0, 1}, p4{3, 2};
    Pieces pieces = {
        {p1, {PieceColor::BLACK, PieceType::PION}},
        {p2, {PieceColor::WHITE, PieceType::PION}}
    };
    Board board(pieces);

    // Black moves
    board.move_piece(p1, p3);
    // White moves
    board.move_piece(p2, p4);
    // Black moves again
    Position p5{1, 2};
    board.move_piece(p3, p5);
    // Promote black
    board.promote_piece(p5);
    REQUIRE(board.is_dame_piece(p5));
    // Remove white
    board.remove_piece(p4);
    REQUIRE(!board.is_occupied(p4));
    REQUIRE(board.get_piece_count() == 1);
}

TEST_CASE("Board copy and move semantics") {
    Position a{1, 0};
    Pieces pieces = {
        {a, {PieceColor::BLACK, PieceType::PION}}
    };
    Board board(pieces);
    Board copy = board;
    REQUIRE(copy.is_occupied(a));
    copy.remove_piece(a);
    REQUIRE(!copy.is_occupied(a));
    REQUIRE(board.is_occupied(a));
    Board moved = std::move(board);
    REQUIRE(moved.is_occupied(a));
}

TEST_CASE("Board edge case chains") {
    std::vector<Position> positions = {
        Position{1, 0}, Position{3, 0}, Position{5, 0}, Position{7, 0},
        Position{0, 1}, Position{2, 1}, Position{4, 1}, Position{6, 1}
    };

    // Add all
    Pieces pieces = {};
    for (auto& pos : positions) pieces[pos] = {PieceColor::BLACK, PieceType::PION};
    
    Board board(pieces);
    REQUIRE(board.get_piece_count() == static_cast<int>(positions.size()));
    
    // Remove all
    for (auto& pos : positions) board.remove_piece(pos);
    REQUIRE(board.get_piece_count() == 0);

    // Add back in reverse
    Pieces reverse_pieces = {};
    for (auto it = positions.rbegin(); it != positions.rend(); ++it)
        reverse_pieces[*it] = {PieceColor::WHITE, PieceType::PION};
    board = Board(reverse_pieces);
    REQUIRE(board.get_piece_count() == static_cast<int>(positions.size()));

    // Move back and forth between two positions
    Position p1 = positions[0], p2 = positions[1];
    board.remove_piece(p2); // Clear destination first
    for (int i = 0; i < 3; ++i) {
        board.move_piece(p1, p2);
        std::swap(p1, p2);
    }

    // Promote, remove
    board.promote_piece(p1);
    board.remove_piece(p1);
    REQUIRE(!board.is_occupied(p1));
}


TEST_CASE("Board reset functionality") {
    Position a{1, 0};
    Position b{3, 0};
    Pieces pieces = {
        {a, {PieceColor::BLACK, PieceType::PION}},
        {b, {PieceColor::WHITE, PieceType::PION}}
    };
    Board board(pieces);

    REQUIRE(board.get_piece_count() == 2);
    // Reset the board
    board.reset();

    // Board should be empty
    REQUIRE(board.get_piece_count() == 0);
    REQUIRE_FALSE(board.is_occupied(a));
    REQUIRE_FALSE(board.is_occupied(b));

    Pieces reinitial_pieces = {
        {a, {PieceColor::BLACK, PieceType::PION}}
    };
    board = Board(reinitial_pieces);
    REQUIRE(board.is_occupied(a));
}

TEST_CASE("Board hash functionality") {
    SECTION("Empty board has zero hash") {
        Board board;
        REQUIRE(board.hash() == 0);
        Board reconstructed = Board::from_hash(0);
        REQUIRE(reconstructed.get_piece_count() == 0);
    }

    SECTION("Hash is consistent and reversible") {
        Pieces pieces = {
            {Position{1, 0}, {PieceColor::BLACK, PieceType::PION}},
            {Position{3, 0}, {PieceColor::WHITE, PieceType::DAME}},
            {Position{0, 1}, {PieceColor::BLACK, PieceType::DAME}}
        };
        Board board(pieces);
        std::size_t hash = board.hash();
        REQUIRE(hash != 0);

        Board reconstructed = Board::from_hash(hash);
        REQUIRE(reconstructed.get_piece_count() == 3);
        REQUIRE(reconstructed.is_occupied(Position{1, 0}));
        REQUIRE(reconstructed.is_black_piece(Position{1, 0}));
        REQUIRE_FALSE(reconstructed.is_dame_piece(Position{1, 0}));

        REQUIRE(reconstructed.is_occupied(Position{3, 0}));
        REQUIRE_FALSE(reconstructed.is_black_piece(Position{3, 0}));
        REQUIRE(reconstructed.is_dame_piece(Position{3, 0}));

        REQUIRE(reconstructed.is_occupied(Position{0, 1}));
        REQUIRE(reconstructed.is_black_piece(Position{0, 1}));
        REQUIRE(reconstructed.is_dame_piece(Position{0, 1}));
    }

    SECTION("Identical boards produce identical hashes") {
        Pieces pieces = {
            {Position{1, 0}, {PieceColor::BLACK, PieceType::PION}},
            {Position{3, 2}, {PieceColor::WHITE, PieceType::PION}}
        };
        Board board1(pieces);
        Board board2(pieces);
        REQUIRE(board1.hash() == board2.hash());
    }

    SECTION("Different boards produce different hashes") {
        Board board_empty;
        Board board_one_piece({ {Position{1, 0}, {PieceColor::BLACK, PieceType::PION}} });
        Board board_one_piece_diff_pos({ {Position{3, 0}, {PieceColor::BLACK, PieceType::PION}} });
        Board board_one_piece_diff_color({ {Position{1, 0}, {PieceColor::WHITE, PieceType::PION}} });
        Board board_one_piece_diff_type({ {Position{1, 0}, {PieceColor::BLACK, PieceType::DAME}} });

        REQUIRE(board_empty.hash() != board_one_piece.hash());
        REQUIRE(board_one_piece.hash() != board_one_piece_diff_pos.hash());
        REQUIRE(board_one_piece.hash() != board_one_piece_diff_color.hash());
        REQUIRE(board_one_piece.hash() != board_one_piece_diff_type.hash());
    }

    SECTION("Hash changes after board modifications") {
        Position p1{1, 0}, p2{0, 1};
        Board board({ {p1, {PieceColor::BLACK, PieceType::PION}} });
        std::size_t initial_hash = board.hash();

        // Move
        board.move_piece(p1, p2);
        std::size_t moved_hash = board.hash();
        REQUIRE(initial_hash != moved_hash);

        // Promote
        board.promote_piece(p2);
        std::size_t promoted_hash = board.hash();
        REQUIRE(moved_hash != promoted_hash);

        // Remove
        board.remove_piece(p2);
        std::size_t removed_hash = board.hash();
        REQUIRE(promoted_hash != removed_hash);
        REQUIRE(removed_hash == 0); // Should be an empty board
    }

    SECTION("Hash reconstruction with maximum 16 pieces") {
        Pieces pieces;
        std::vector<Position> positions;
        for (int y = 0; y < 4; ++y) {
            for (int x = (y % 2 == 0 ? 1 : 0); x < 8; x += 2) {
                positions.push_back(Position{x, y});
            }
        }
        
        for(size_t i = 0; i < positions.size(); ++i) {
            pieces[positions[i]] = { (i % 2 == 0) ? PieceColor::BLACK : PieceColor::WHITE, (i % 3 == 0) ? PieceType::DAME : PieceType::PION };
        }

        Board board(pieces);
        REQUIRE(board.get_piece_count() == 16);

        std::size_t hash = board.hash();
        Board reconstructed = Board::from_hash(hash);

        REQUIRE(reconstructed.get_piece_count() == 16);
        for(const auto& pos : positions) {
            REQUIRE(reconstructed.is_occupied(pos));
            REQUIRE(reconstructed.is_black_piece(pos) == board.is_black_piece(pos));
            REQUIRE(reconstructed.is_dame_piece(pos) == board.is_dame_piece(pos));
        }
    }
}

TEST_CASE("Board setup functionality") {
    SECTION("Setup creates standard initial board configuration") {
        Board board = Board::setup();
        
        // Verify total piece count (8 white + 8 black = 16 pieces)
        REQUIRE(board.get_piece_count() == 16);
        REQUIRE(board.get_piece_count(PieceColor::WHITE) == 8);
        REQUIRE(board.get_piece_count(PieceColor::BLACK) == 8);
    }

    SECTION("Setup places black pieces correctly on rows 0-1") {
        Board board = Board::setup();
        
        // Row 0: black pieces at columns 1, 3, 5, 7 (row is even, start_col = 1)
        REQUIRE(board.is_occupied(Position::from_coords(1, 0)));
        REQUIRE(board.is_black_piece(Position::from_coords(1, 0)));
        REQUIRE_FALSE(board.is_dame_piece(Position::from_coords(1, 0)));
        
        REQUIRE(board.is_occupied(Position::from_coords(3, 0)));
        REQUIRE(board.is_black_piece(Position::from_coords(3, 0)));
        
        REQUIRE(board.is_occupied(Position::from_coords(5, 0)));
        REQUIRE(board.is_black_piece(Position::from_coords(5, 0)));
        
        REQUIRE(board.is_occupied(Position::from_coords(7, 0)));
        REQUIRE(board.is_black_piece(Position::from_coords(7, 0)));
        
        // Row 1: black pieces at columns 0, 2, 4, 6 (row is odd, start_col = 0)
        REQUIRE(board.is_occupied(Position::from_coords(0, 1)));
        REQUIRE(board.is_black_piece(Position::from_coords(0, 1)));
        
        REQUIRE(board.is_occupied(Position::from_coords(2, 1)));
        REQUIRE(board.is_black_piece(Position::from_coords(2, 1)));
        
        REQUIRE(board.is_occupied(Position::from_coords(4, 1)));
        REQUIRE(board.is_black_piece(Position::from_coords(4, 1)));
        
        REQUIRE(board.is_occupied(Position::from_coords(6, 1)));
        REQUIRE(board.is_black_piece(Position::from_coords(6, 1)));
    }

    SECTION("Setup places white pieces correctly on rows 6-7") {
        Board board = Board::setup();
        
        // Row 6: white pieces at columns 1, 3, 5, 7 (row is even, start_col = 1)
        REQUIRE(board.is_occupied(Position::from_coords(1, 6)));
        REQUIRE_FALSE(board.is_black_piece(Position::from_coords(1, 6)));
        REQUIRE_FALSE(board.is_dame_piece(Position::from_coords(1, 6)));
        
        REQUIRE(board.is_occupied(Position::from_coords(3, 6)));
        REQUIRE_FALSE(board.is_black_piece(Position::from_coords(3, 6)));
        
        REQUIRE(board.is_occupied(Position::from_coords(5, 6)));
        REQUIRE_FALSE(board.is_black_piece(Position::from_coords(5, 6)));
        
        REQUIRE(board.is_occupied(Position::from_coords(7, 6)));
        REQUIRE_FALSE(board.is_black_piece(Position::from_coords(7, 6)));
        
        // Row 7: white pieces at columns 0, 2, 4, 6 (row is odd, start_col = 0)
        REQUIRE(board.is_occupied(Position::from_coords(0, 7)));
        REQUIRE_FALSE(board.is_black_piece(Position::from_coords(0, 7)));
        
        REQUIRE(board.is_occupied(Position::from_coords(2, 7)));
        REQUIRE_FALSE(board.is_black_piece(Position::from_coords(2, 7)));
        
        REQUIRE(board.is_occupied(Position::from_coords(4, 7)));
        REQUIRE_FALSE(board.is_black_piece(Position::from_coords(4, 7)));
        
        REQUIRE(board.is_occupied(Position::from_coords(6, 7)));
        REQUIRE_FALSE(board.is_black_piece(Position::from_coords(6, 7)));
    }

    SECTION("Setup leaves middle rows empty") {
        Board board = Board::setup();
        
        // Rows 2, 3, 4, 5 should be completely empty (only check valid positions)
        for (int row = 2; row <= 5; ++row) {
            for (int col = 0; col < 8; ++col) {
                // Only create position if it would be valid
                if (Position::is_valid(col, row)) {
                    Position pos = Position::from_coords(col, row);
                    REQUIRE_FALSE(board.is_occupied(pos));
                }
            }
        }
    }

    SECTION("Setup places pieces only on valid checkerboard squares") {
        Board board = Board::setup();
        
        // Count all pieces and verify they're only on valid squares
        int total_pieces = 0;
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                // Only create position if it would be valid
                if (Position::is_valid(col, row)) {
                    Position pos = Position::from_coords(col, row);
                    if (board.is_occupied(pos)) {
                        total_pieces++;
                    }
                }
            }
        }
        REQUIRE(total_pieces == 16);
    }

    SECTION("Setup creates only PION pieces, no DAME pieces") {
        Board board = Board::setup();
        
        // Check all black pieces are PION (rows 0-1)
        for (int row = 0; row <= 1; ++row) {
            const int start_col = (row % 2 == 0) ? 1 : 0;
            for (int col = start_col; col < 8; col += 2) {
                Position pos = Position::from_coords(col, row);
                REQUIRE(board.is_occupied(pos));
                REQUIRE_FALSE(board.is_dame_piece(pos));
            }
        }
        
        // Check all white pieces are PION (rows 6-7)
        for (int row = 6; row <= 7; ++row) {
            const int start_col = (row % 2 == 0) ? 1 : 0;
            for (int col = start_col; col < 8; col += 2) {
                Position pos = Position::from_coords(col, row);
                REQUIRE(board.is_occupied(pos));
                REQUIRE_FALSE(board.is_dame_piece(pos));
            }
        }
    }

    SECTION("Setup is consistent across multiple calls") {
        Board board1 = Board::setup();
        Board board2 = Board::setup();
        
        // Both boards should be identical
        REQUIRE(board1.get_piece_count() == board2.get_piece_count());
        REQUIRE(board1.get_piece_count(PieceColor::WHITE) == board2.get_piece_count(PieceColor::WHITE));
        REQUIRE(board1.get_piece_count(PieceColor::BLACK) == board2.get_piece_count(PieceColor::BLACK));
        
        // Check that all valid positions match
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                // Only create position if it would be valid
                if (Position::is_valid(col, row)) {
                    Position pos = Position::from_coords(col, row);
                    REQUIRE(board1.is_occupied(pos) == board2.is_occupied(pos));
                    if (board1.is_occupied(pos)) {
                        REQUIRE(board1.is_black_piece(pos) == board2.is_black_piece(pos));
                        REQUIRE(board1.is_dame_piece(pos) == board2.is_dame_piece(pos));
                    }
                }
            }
        }
    }

    SECTION("Setup board can be used for game operations") {
        Board board = Board::setup();
        
        // Test moving a black piece forward (from row 0 to row 2)
        Position black_start = Position::from_coords(1, 0);
        Position black_move = Position::from_coords(0, 3);  // Move to empty middle area
        
        // Move the piece
        REQUIRE_NOTHROW(board.move_piece(black_start, black_move));
        REQUIRE_FALSE(board.is_occupied(black_start));
        REQUIRE(board.is_occupied(black_move));
        REQUIRE(board.is_black_piece(black_move));
        
        // Test promoting a piece
        Position promotion_pos = Position::from_coords(2, 7);
        REQUIRE_NOTHROW(board.promote_piece(promotion_pos));
        REQUIRE(board.is_dame_piece(promotion_pos));
        
        // Total piece count should be correct (no pieces removed)
        REQUIRE(board.get_piece_count() == 16);
    }

    SECTION("Setup board hash is non-zero and reproducible") {
        Board board = Board::setup();
        std::size_t hash1 = board.hash();
        std::size_t hash2 = Board::setup().hash();
        
        // Setup board should have a non-zero hash
        REQUIRE(hash1 != 0);
        
        // Hash should be consistent
        REQUIRE(hash1 == hash2);
        
        // Hash should be reversible
        Board reconstructed = Board::from_hash(hash1);
        REQUIRE(reconstructed.get_piece_count() == 16);
        REQUIRE(reconstructed.get_piece_count(PieceColor::WHITE) == 8);
        REQUIRE(reconstructed.get_piece_count(PieceColor::BLACK) == 8);
    }
}

TEST_CASE("Board static methods and validation") {
    SECTION("is_valid_position correctly validates positions") {
        // Valid positions (dark squares where x + y is odd)
        REQUIRE(Board::is_valid_position(Position::from_coords(1, 0)));
        REQUIRE(Board::is_valid_position(Position::from_coords(3, 0)));
        REQUIRE(Board::is_valid_position(Position::from_coords(0, 1)));
        REQUIRE(Board::is_valid_position(Position::from_coords(2, 1)));
        REQUIRE(Board::is_valid_position(Position::from_coords(7, 6)));
        REQUIRE(Board::is_valid_position(Position::from_coords(6, 7)));

        // Note: Invalid positions cannot be created through Position::from_coords as it validates
        // So we test the static method directly with direct Position construction
        // The is_valid_position method in Board checks both Position::is_valid AND valid checkerboard squares
        
        // Test with valid Position objects - all should return true since Position validates on construction
        Position valid_pos1 = Position::from_coords(1, 0);
        Position valid_pos2 = Position::from_coords(7, 6);
        REQUIRE(Board::is_valid_position(valid_pos1));
        REQUIRE(Board::is_valid_position(valid_pos2));
    }

    SECTION("copy static method creates independent copy") {
        Position p1{1, 0}, p2{3, 0};
        Pieces pieces = {
            {p1, {PieceColor::BLACK, PieceType::PION}},
            {p2, {PieceColor::WHITE, PieceType::DAME}}
        };
        Board original(pieces);
        Board copied = Board::copy(original);

        REQUIRE(copied.get_piece_count() == 2);
        REQUIRE(copied.is_occupied(p1));
        REQUIRE(copied.is_black_piece(p1));
        REQUIRE(copied.is_occupied(p2));
        REQUIRE_FALSE(copied.is_black_piece(p2));
        REQUIRE(copied.is_dame_piece(p2));

        // Modify original - copy should remain unchanged
        original.remove_piece(p1);
        REQUIRE_FALSE(original.is_occupied(p1));
        REQUIRE(copied.is_occupied(p1));  // Copy should still have the piece
    }
}

TEST_CASE("Board optional-returning methods") {
    Position valid_pos{1, 0};
    Position empty_pos{3, 0};
    
    Pieces pieces = {
        {valid_pos, {PieceColor::BLACK, PieceType::DAME}}
    };
    Board board(pieces);

    SECTION("get_piece_color returns correct values") {
        // Valid position with piece
        auto color = board.get_piece_color(valid_pos);
        REQUIRE(color.has_value());
        REQUIRE(color.value() == PieceColor::BLACK);

        // Valid position without piece
        auto empty_color = board.get_piece_color(empty_pos);
        REQUIRE_FALSE(empty_color.has_value());
    }

    SECTION("get_piece_type returns correct values") {
        // Valid position with piece
        auto type = board.get_piece_type(valid_pos);
        REQUIRE(type.has_value());
        REQUIRE(type.value() == PieceType::DAME);

        // Valid position without piece
        auto empty_type = board.get_piece_type(empty_pos);
        REQUIRE_FALSE(empty_type.has_value());
    }

    SECTION("get_piece_info returns correct values") {
        // Valid position with piece
        auto info = board.get_piece_info(valid_pos);
        REQUIRE(info.has_value());
        REQUIRE(info.value().color == PieceColor::BLACK);
        REQUIRE(info.value().type == PieceType::DAME);

        // Valid position without piece
        auto empty_info = board.get_piece_info(empty_pos);
        REQUIRE_FALSE(empty_info.has_value());
    }
}

TEST_CASE("Board error handling for invalid positions and edge cases") {
    Position valid_pos{1, 0};
    Position empty_pos{3, 0};
    
    Pieces pieces = {
        {valid_pos, {PieceColor::BLACK, PieceType::PION}}
    };
    Board board(pieces);

    SECTION("is_black_piece throws for empty positions") {
        REQUIRE_THROWS_AS(board.is_black_piece(empty_pos), std::invalid_argument);
        REQUIRE_NOTHROW(board.is_black_piece(valid_pos));
    }

    SECTION("is_dame_piece throws for empty positions") {
        REQUIRE_THROWS_AS(board.is_dame_piece(empty_pos), std::invalid_argument);
        REQUIRE_NOTHROW(board.is_dame_piece(valid_pos));
    }

    SECTION("promote_piece throws for empty positions") {
        REQUIRE_THROWS_AS(board.promote_piece(empty_pos), std::invalid_argument);
        REQUIRE_NOTHROW(board.promote_piece(valid_pos));
    }

    SECTION("move_piece throws for various invalid scenarios") {
        Position valid_dest{0, 1};
        REQUIRE_THROWS_AS(board.move_piece(empty_pos, valid_dest), std::invalid_argument);
        
        // Test moving to same position - this should NOT throw, it returns early
        REQUIRE_NOTHROW(board.move_piece(valid_pos, valid_pos));
        
        // Test moving to occupied position - create separate board with two pieces
        Position pos1{1, 0}, pos2{3, 0};
        Pieces occupied_pieces = {
            {pos1, {PieceColor::BLACK, PieceType::PION}},
            {pos2, {PieceColor::WHITE, PieceType::PION}}
        };
        Board occupied_board(occupied_pieces);
        
        // Try to move one piece to the position of another
        REQUIRE_THROWS_AS(occupied_board.move_piece(pos1, pos2), std::invalid_argument);
    }
}

TEST_CASE("Board get_pieces by color") {
    Position black_pos1{1, 0}, black_pos2{3, 0};
    Position white_pos1{0, 1}, white_pos2{2, 1};
    
    Pieces pieces = {
        {black_pos1, {PieceColor::BLACK, PieceType::PION}},
        {black_pos2, {PieceColor::BLACK, PieceType::DAME}},
        {white_pos1, {PieceColor::WHITE, PieceType::PION}},
        {white_pos2, {PieceColor::WHITE, PieceType::DAME}}
    };
    Board board(pieces);

    SECTION("get_pieces returns correct color pieces") {
        auto black_pieces = board.get_pieces(PieceColor::BLACK);
        auto white_pieces = board.get_pieces(PieceColor::WHITE);

        REQUIRE(black_pieces.size() == 2);
        REQUIRE(white_pieces.size() == 2);

        // Check black pieces
        REQUIRE(black_pieces.contains(black_pos1));
        REQUIRE(black_pieces.contains(black_pos2));
        REQUIRE(black_pieces[black_pos1].color == PieceColor::BLACK);
        REQUIRE(black_pieces[black_pos2].color == PieceColor::BLACK);
        REQUIRE(black_pieces[black_pos1].type == PieceType::PION);
        REQUIRE(black_pieces[black_pos2].type == PieceType::DAME);

        // Check white pieces
        REQUIRE(white_pieces.contains(white_pos1));
        REQUIRE(white_pieces.contains(white_pos2));
        REQUIRE(white_pieces[white_pos1].color == PieceColor::WHITE);
        REQUIRE(white_pieces[white_pos2].color == PieceColor::WHITE);
        REQUIRE(white_pieces[white_pos1].type == PieceType::PION);
        REQUIRE(white_pieces[white_pos2].type == PieceType::DAME);
    }

    SECTION("get_pieces returns empty for color with no pieces") {
        Board empty_board;
        auto black_pieces = empty_board.get_pieces(PieceColor::BLACK);
        auto white_pieces = empty_board.get_pieces(PieceColor::WHITE);

        REQUIRE(black_pieces.empty());
        REQUIRE(white_pieces.empty());
    }
}

TEST_CASE("Board assignment operators") {
    Position p1{1, 0}, p2{3, 0};
    Pieces pieces = {
        {p1, {PieceColor::BLACK, PieceType::PION}},
        {p2, {PieceColor::WHITE, PieceType::DAME}}
    };

    SECTION("copy assignment operator") {
        Board original(pieces);
        Board copy;
        
        REQUIRE(copy.get_piece_count() == 0);
        copy = original;
        REQUIRE(copy.get_piece_count() == 2);
        REQUIRE(copy.is_occupied(p1));
        REQUIRE(copy.is_occupied(p2));

        // Modify original, copy should remain unchanged
        original.remove_piece(p1);
        REQUIRE_FALSE(original.is_occupied(p1));
        REQUIRE(copy.is_occupied(p1));
    }

    SECTION("move assignment operator") {
        Board original(pieces);
        Board moved;
        
        REQUIRE(moved.get_piece_count() == 0);
        moved = std::move(original);
        REQUIRE(moved.get_piece_count() == 2);
        REQUIRE(moved.is_occupied(p1));
        REQUIRE(moved.is_occupied(p2));
    }
}

TEST_CASE("Board equality operator") {
    Position p1{1, 0}, p2{3, 0};
    Pieces pieces1 = {
        {p1, {PieceColor::BLACK, PieceType::PION}},
        {p2, {PieceColor::WHITE, PieceType::DAME}}
    };
    Pieces pieces2 = {
        {p1, {PieceColor::BLACK, PieceType::PION}},
        {p2, {PieceColor::WHITE, PieceType::DAME}}
    };
    Pieces pieces3 = {
        {p1, {PieceColor::BLACK, PieceType::PION}}
    };

    SECTION("identical boards are equal") {
        Board board1(pieces1);
        Board board2(pieces2);
        REQUIRE(board1 == board2);
    }

    SECTION("different boards are not equal") {
        Board board1(pieces1);
        Board board3(pieces3);
        REQUIRE_FALSE(board1 == board3);
    }

    SECTION("empty boards are equal") {
        Board empty1;
        Board empty2;
        REQUIRE(empty1 == empty2);
    }

    SECTION("board equals itself") {
        Board board(pieces1);
        REQUIRE(board == board);
    }
}

TEST_CASE("Board iterator and range access") {
    Position p1{1, 0}, p2{3, 0}, p3{0, 1};
    Pieces pieces = {
        {p1, {PieceColor::BLACK, PieceType::PION}},
        {p2, {PieceColor::WHITE, PieceType::DAME}},
        {p3, {PieceColor::BLACK, PieceType::DAME}}
    };
    Board board(pieces);

    SECTION("pieces_view returns const reference") {
        const auto& pieces_ref = board.pieces_view();
        REQUIRE(pieces_ref.size() == 3);
        REQUIRE(pieces_ref.contains(p1));
        REQUIRE(pieces_ref.contains(p2));
        REQUIRE(pieces_ref.contains(p3));
    }

    SECTION("get_pieces returns const reference") {
        const auto& pieces_ref = board.get_pieces();
        REQUIRE(pieces_ref.size() == 3);
        REQUIRE(pieces_ref.contains(p1));
        REQUIRE(pieces_ref.contains(p2));
        REQUIRE(pieces_ref.contains(p3));
    }

    SECTION("iterator access works") {
        int count = 0;
        for (auto it = board.begin(); it != board.end(); ++it) {
            count++;
            REQUIRE(pieces.contains(it->first));
        }
        REQUIRE(count == 3);
    }

    SECTION("range-based for loop works") {
        int count = 0;
        std::unordered_map<Position, PieceInfo> found_pieces;
        
        for (const auto& [position, piece_info] : board) {
            count++;
            found_pieces[position] = piece_info;
        }
        
        REQUIRE(count == 3);
        REQUIRE(found_pieces.size() == 3);
        REQUIRE(found_pieces[p1].color == PieceColor::BLACK);
        REQUIRE(found_pieces[p1].type == PieceType::PION);
        REQUIRE(found_pieces[p2].color == PieceColor::WHITE);
        REQUIRE(found_pieces[p2].type == PieceType::DAME);
        REQUIRE(found_pieces[p3].color == PieceColor::BLACK);
        REQUIRE(found_pieces[p3].type == PieceType::DAME);
    }
}

TEST_CASE("Board edge cases and special scenarios") {
    SECTION("promote already promoted piece does nothing") {
        Position p1{1, 0};
        Pieces pieces = {{p1, {PieceColor::BLACK, PieceType::DAME}}};
        Board board(pieces);

        REQUIRE(board.is_dame_piece(p1));
        REQUIRE_NOTHROW(board.promote_piece(p1));
        REQUIRE(board.is_dame_piece(p1));  // Should still be DAME
    }

    SECTION("move piece to same position does nothing") {
        Position p1{1, 0};
        Pieces pieces = {{p1, {PieceColor::BLACK, PieceType::PION}}};
        Board board(pieces);

        REQUIRE_NOTHROW(board.move_piece(p1, p1));
        REQUIRE(board.is_occupied(p1));
        REQUIRE(board.get_piece_count() == 1);
    }

    SECTION("remove piece from empty position is safe") {
        Board board;
        Position empty_pos{1, 0};
        
        REQUIRE_FALSE(board.is_occupied(empty_pos));
        REQUIRE_NOTHROW(board.remove_piece(empty_pos));
        REQUIRE_FALSE(board.is_occupied(empty_pos));
    }

    SECTION("remove piece from any position is safe") {
        Board board;
        Position any_pos{3, 2};  // Valid checkerboard position
        
        REQUIRE_NOTHROW(board.remove_piece(any_pos));
    }

    SECTION("large board operations maintain consistency") {
        // Create a board with many pieces
        Pieces pieces;
        std::vector<Position> positions;
        
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (Position::is_valid(x, y)) {  // Use Position::is_valid instead of Board::is_valid_position
                    Position pos = Position::from_coords(x, y);
                    positions.push_back(pos);
                    pieces[pos] = {(positions.size() % 2) ? PieceColor::BLACK : PieceColor::WHITE, PieceType::PION};
                }
            }
        }
        
        Board board(pieces);
        REQUIRE(board.get_piece_count() == static_cast<int>(positions.size()));
        REQUIRE(board.get_piece_count() == 32);  // All valid squares on a checkerboard

        // Test all pieces are accessible
        for (const auto& pos : positions) {
            REQUIRE(board.is_occupied(pos));
            REQUIRE_NOTHROW(board.get_piece_color(pos));
            REQUIRE_NOTHROW(board.get_piece_type(pos));
        }

        // Remove half the pieces
        for (size_t i = 0; i < positions.size() / 2; i++) {
            board.remove_piece(positions[i]);
        }
        REQUIRE(board.get_piece_count() == static_cast<int>(positions.size() / 2));
    }
}

TEST_CASE("Board hash consistency and std::hash specialization") {
    Position p1{1, 0}, p2{3, 0};
    Pieces pieces = {
        {p1, {PieceColor::BLACK, PieceType::PION}},
        {p2, {PieceColor::WHITE, PieceType::DAME}}
    };

    SECTION("std::hash specialization works") {
        Board board(pieces);
        std::hash<Board> hasher;
        
        std::size_t hash1 = hasher(board);
        std::size_t hash2 = board.hash();
        
        REQUIRE(hash1 == hash2);
        REQUIRE(hash1 != 0);
    }

    SECTION("boards with same content have same hash") {
        Board board1(pieces);
        Board board2(pieces);
        
        std::hash<Board> hasher;
        REQUIRE(hasher(board1) == hasher(board2));
        REQUIRE(board1.hash() == board2.hash());
    }

    SECTION("modified boards have different hashes") {
        Board board1(pieces);
        Board board2(pieces);
        
        board2.remove_piece(p1);
        
        std::hash<Board> hasher;
        REQUIRE(hasher(board1) != hasher(board2));
        REQUIRE(board1.hash() != board2.hash());
    }
}