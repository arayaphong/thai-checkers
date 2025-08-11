#pragma once

#include <iostream>
#include <cstddef>
#include "Game.h"

class Traversal {
public:
    Traversal() {}

    void traverse(const Game& game = Game()) {
        const auto& move_count = game.move_count();
        if (looping_database.contains(game.board().hash())) {
            return;
        }

        if (move_count == 0) {
            std::cout << "[" << game_count << "] ";
            const auto& history = game.get_move_sequence();
            const auto& move_history_count = history.size() / 2;
            const auto& is_looping = game.is_looping();
            if (is_looping) {
                looping_database.insert(game.board().hash());
                std::cout << move_history_count << " Moves ended in a loop!" << std::endl;
            } else {
                const auto& winner = (game.player() == PieceColor::BLACK ? "White" : "Black");
                std::cout << move_history_count << " Moves ended with a " << winner << " win!" << std::endl;
            }
            ++game_count;
            return;
        }

        for (std::size_t i = 0; i < move_count; ++i) {
            auto next_game = Game::copy(game);
            next_game.select_move(i);
            traverse(next_game);
        }
    }

private:
    std::size_t game_count = 1;
    std::unordered_set<std::size_t> looping_database;
};
