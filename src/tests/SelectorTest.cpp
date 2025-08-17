// Catch2
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

#include "Game.h"

namespace { // anonymous namespace for test translation unit (satisfies misc-use-anonymous-namespace)

TEST_CASE("Game plays to completion with first-move selector", "[selector][determinism]") {
    Game game;
    while (game.move_count() != 0) { game.select_move(0); }
    const auto& move_sequence = game.get_move_sequence();
    const std::vector<uint8_t> expected = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    REQUIRE(move_sequence == expected);
}

TEST_CASE("Game plays to completion with last-move selector", "[selector][determinism]") {
    Game game;
    while (game.move_count() != 0) { game.select_move(game.move_count() - 1); }
    const auto& move_sequence = game.get_move_sequence();
    const std::vector<uint8_t> expected = {6, 6, 7, 7, 6, 8, 6, 6, 5, 8, 6, 7, 1, 7, 1, 6,  0, 1, 6,
                                           5, 6, 6, 1, 4, 0, 3, 7, 3, 6, 3, 0, 2, 0, 0, 13, 1, 0};
    REQUIRE(move_sequence == expected);
}

TEST_CASE("Game plays to completion with middle-move selector", "[selector][determinism]") {
    Game game;
    while (true) {
        const auto move_count = game.move_count();
        if (move_count == 0) { break; }
        game.select_move(move_count / 2);
    }
    const auto& move_sequence = game.get_move_sequence();
    const std::vector<uint8_t> expected = {3, 3, 4, 4, 5, 5, 6, 6, 5, 6, 5, 6, 6, 6, 6, 6, 0, 5, 7, 6, 6, 1, 6, 0, 5,
                                           1, 4, 7, 4, 4, 0, 3, 0, 2, 0, 4, 0, 1, 5, 1, 5, 1, 0, 0, 5, 1, 8, 0, 0};
    REQUIRE(move_sequence == expected);
}

TEST_CASE("Game plays to completion with alternating selector", "[selector][determinism]") {
    Game game;
    int state = 0;
    while (true) {
        const auto move_count = game.move_count();
        if (move_count == 0) { break; }
        const auto select_index = (state % 2 == 0) ? move_count - 1 : 0;
        game.select_move(select_index);
        ++state;
    }
    const auto& move_sequence = game.get_move_sequence();
    const std::vector<uint8_t> expected = {6, 0, 7, 0, 6, 0, 7, 0, 6, 0, 7, 0, 6, 0, 7, 0, 6, 0, 7,
                                           0, 6, 0, 7, 0, 6, 0, 5, 0, 4, 0, 0, 0, 5, 0, 5, 0, 4, 0,
                                           3, 0, 2, 0, 1, 0, 1, 0, 5, 0, 2, 0, 1, 0, 7, 0, 0, 0};
    REQUIRE(move_sequence == expected);
}

} // namespace
