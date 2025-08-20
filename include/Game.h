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
    std::vector<uint8_t> index_history;
    std::vector<Board> board_history; // Store complete board states for proper undo

    mutable bool is_looping_ = false;
    mutable bool choices_dirty_ = true;
    mutable std::vector<Move> choices_cache_{};

  public:
    static Game copy(const Game& other) { return other; }

  private:
    std::unordered_map<Position, Legals> get_moveable_pieces() const;
    const std::vector<Move>& get_choices() const;
    void execute_move(const Move& move);
    bool seen(const Board& board) const noexcept;

  public:
    Game() noexcept : choices_dirty_(true), choices_cache_{} {
        board_history.push_back(Board::setup()); // Initialize with starting position
    }
    Game(Board b) noexcept : choices_dirty_(true), choices_cache_{} {
        board_history.push_back(b); // Initialize with given position
    }

    [[nodiscard]] std::size_t move_count() const { return get_choices().size(); }
    void undo_move();
    void select_move(std::size_t index);
    void print_board() const noexcept;
    void print_choices() const;

    // Accessors
    [[nodiscard]] const std::vector<uint8_t>& get_move_sequence() const noexcept { return index_history; }
    [[nodiscard]] bool is_looping() const noexcept { return is_looping_; }

    [[nodiscard]] const Board& board() const noexcept { return board_history.back(); }
    [[nodiscard]] PieceColor player() const noexcept;
};
