#pragma once

#include <iostream>
#include <cstddef>
#include "Game.h"

class Traversal {
public:
    Traversal() {}

    void print_selection_history(const Game& game) const {
        std::cout << "Selection History:" << std::endl;
        const auto& history = game.get_move_sequence();
        for (std::size_t i = 1; i < history.size(); i+=2) {
            std::cout << history[i] << ".";
        }
        std::cout << std::endl;
    }

    void traverse(std::size_t move_index, int turn_count = 1, Game game = Game()) {
        if (looping_database.contains(game.board().hash())) {
            return; // Avoid cycles
        }

        // Debug output
        std::cout << std::endl;
        const std::string& player = game.player() == PieceColor::BLACK ? "Black" : "White";
        std::cout << "[" << game_count << ":" << turn_count << "] " << player << std::endl;
        game.print_board();
        game.print_choices();
        ++turn_count;
        // std::cin.ignore();

        game.select_move(move_index);

        const auto& move_count = game.move_count();
        if (move_count == 0) {
            std::cout << std::endl;
            const std::string& player = game.player() == PieceColor::BLACK ? "Black" : "White";
            std::cout << "[" << game_count << ":" << turn_count << "] " << player << std::endl;
            game.print_board();
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
            const auto& next_game = Game::copy(game);
            traverse(i, turn_count, next_game);
        }
    }

private:
    std::size_t game_count = 1;
    std::unordered_set<std::size_t> looping_database;
};
