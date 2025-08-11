#pragma once

#include "Board.h"
#include "Piece.h"
#include "Position.h"
#include "Legals.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>


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
    std::vector<std::size_t> board_move_sequence;

    mutable std::vector<Move> choices_cache_{};
    mutable bool choices_dirty_ = true;
    // Repetition detection and termination
    std::unordered_set<std::size_t> seen_states_{}; // key = board hash ^ mix(player)
    bool game_over_ = false;
    [[nodiscard]] std::size_t state_key() const noexcept {
        return current_board.hash() ^ (static_cast<std::size_t>(current_player) * 0x9e3779b97f4a7c15ULL);
    }
private:
    std::unordered_map<Position, Legals> get_moveable_pieces() const;
    const std::vector<Move>& get_choices() const;
    Board execute_move(const Move& move);
public:
    Game();
    [[nodiscard]] std::size_t move_count() const;
    void select_move(std::size_t index);
    void print_board() const noexcept;
    void print_choices() const;

    // Accessors
    [[nodiscard]] const std::vector<std::size_t>& get_move_sequence() const noexcept { return board_move_sequence; }
    [[nodiscard]] const Board& board() const noexcept { return current_board; }
    [[nodiscard]] PieceColor player() const noexcept { return current_player; }
};
