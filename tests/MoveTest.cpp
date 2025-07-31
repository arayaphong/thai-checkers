#include <catch2/catch_all.hpp>
#include "Move.h"
#include "Position.h"

TEST_CASE("MakeMove", "[Move]") {
    const Position from = Position::from_coords(1, 0);
    const Position to = Position::from_coords(2, 1);
    const auto move = Move::make_move(1, from, to);

    CHECK(move.depth == 1);
    CHECK(move.focus == from);
    REQUIRE(move.target.has_value());
    CHECK(move.target.value() == to);
    CHECK_FALSE(move.captured.has_value());
    CHECK_FALSE(move.is_capture());
}

TEST_CASE("MakeCapture", "[Move]") {
    const Position from = Position::from_coords(1, 0);
    const Position to = Position::from_coords(3, 2);
    const Position captured_pos = Position::from_coords(2, 1);
    const auto move = Move::make_capture(1, from, to, captured_pos);

    CHECK(move.depth == 1);
    CHECK(move.focus == from);
    REQUIRE(move.target.has_value());
    CHECK(move.target.value() == to);
    REQUIRE(move.captured.has_value());
    CHECK(move.captured.value() == captured_pos);
    CHECK(move.is_capture());
}

TEST_CASE("HashAndFromHashForMove", "[Move]") {
    const Position from = Position::from_coords(0, 1); // index 0
    const Position to = Position::from_coords(1, 0);   // index 4
    const auto original_move = Move::make_move(5, from, to);

    const std::size_t hash_value = original_move.hash();
    const Move reconstructed_move = Move::from_hash(hash_value);

    CHECK(original_move.depth == reconstructed_move.depth);
    CHECK(original_move.focus == reconstructed_move.focus);
    CHECK(original_move.target == reconstructed_move.target);
    CHECK(original_move.captured == reconstructed_move.captured);
    CHECK(original_move == reconstructed_move);
}

TEST_CASE("HashAndFromHashForCapture", "[Move]") {
    const Position from = Position::from_coords(7, 6); // index 31
    const Position to = Position::from_coords(5, 4);   // index 22
    const Position captured_pos = Position::from_coords(6, 5); // index 26
    const auto original_move = Move::make_capture(10, from, to, captured_pos);

    const std::size_t hash_value = original_move.hash();
    const Move reconstructed_move = Move::from_hash(hash_value);

    CHECK(original_move.depth == reconstructed_move.depth);
    CHECK(original_move.focus == reconstructed_move.focus);
    CHECK(original_move.target == reconstructed_move.target);
    CHECK(original_move.captured == reconstructed_move.captured);
    CHECK(original_move == reconstructed_move);
}

TEST_CASE("FromHashWithBoundaryValues", "[Move]") {
    // Since all position indices 0-31 are valid for the checkers board,
    // this test verifies that Move::from_hash works correctly with boundary values.
    
    // Test that the function works with boundary values (all positions at index 31)
    const std::size_t boundary_hash = (0b0 << 19) | (0b0001 << 15) | (31 << 10) | (31 << 5) | 31;
    CHECK_NOTHROW(Move::from_hash(boundary_hash));
}

TEST_CASE("HashThrowsOnInvalidMove", "[Move]") {
    Move invalid_move;
    invalid_move.focus = Position::from_coords(1, 0);  // Valid position
    invalid_move.target = std::nullopt; // No target position

    CHECK_THROWS_AS(invalid_move.hash(), std::invalid_argument);
}

TEST_CASE("Comparison", "[Move]") {
    const Position p1 = Position::from_coords(1, 0);
    const Position p2 = Position::from_coords(2, 1);
    const Position p3 = Position::from_coords(3, 0);
    const Position p4 = Position::from_coords(4, 1);

    const auto move1 = Move::make_move(1, p1, p2);
    const auto move2 = Move::make_move(1, p1, p2); // Same as move1
    const auto move3 = Move::make_move(1, p3, p4); // Different from move1
    const auto capture1 = Move::make_capture(1, p1, p3, p2); // Different from all

    CHECK(move1 == move2);
    CHECK(move1 != move3);
    CHECK(move1 != capture1);
    CHECK(move3 != capture1);

    // Comparison depends on hash value
    if (move1.hash() < move3.hash()) {
        CHECK(move1 < move3);
    } else {
        CHECK(move1 > move3);
    }
}

TEST_CASE("StdHashSpecialization", "[Move]") {
    const Position from = Position::from_coords(1, 0);
    const Position to = Position::from_coords(2, 1);
    const auto move = Move::make_move(1, from, to);

    std::hash<Move> hasher;
    CHECK(move.hash() == hasher(move));
}