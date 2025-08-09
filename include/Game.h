#pragma once

#include "Board.h"
#include "Piece.h"
#include "Position.h"
#include "Options.h"
#include <string>
#include <unordered_set>
#include <vector>
#include <optional>
#include <algorithm>


// Function declarations
std::string piece_symbol(bool is_black, bool is_dame);
std::string board_to_string(const Board& board);

// Strongly-typed move representation replacing nested pair structure.
struct Move {
    Position from;
    Position to;
    std::vector<Position> captured; // empty if non-capture

    [[nodiscard]] bool is_capture() const noexcept { return !captured.empty(); }
    [[nodiscard]] bool operator==(const Move& other) const noexcept {
        return from == other.from && to == other.to && captured == other.captured;
    }
};

class Game {
    PieceColor current_player;
    Board current_board;
    std::unordered_set<Board> model;
private:
    std::unordered_map<Position, Options> get_moveable_pieces() const;
    [[nodiscard]] bool is_legal_move(const Move& move) const; // validation helper

public:
    Game();
    std::vector<Move> get_choices() const;
    Board execute_move(const Move& move);
    void print_board() const;

    // Accessors
    [[nodiscard]] const Board& board() const noexcept { return current_board; }
    [[nodiscard]] PieceColor player() const noexcept { return current_player; }
};
