
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