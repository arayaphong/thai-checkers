#pragma once
#include <memory>
#include <map>

#include "Position.h"
#include "Piece.h"
using Pieces = std::map<Position, PieceInfo>;

class Board {
public:
    Board();
    ~Board();
    Board(const Board&);
    Board(Board&&) noexcept;
    Board(const Pieces& pieces);

    static bool is_valid_position(const Position& pos);
    static Board from_hash(std::size_t hash_value);

    std::size_t hash() const noexcept;
    Board& operator=(Board&&) noexcept;
    Board &operator=(const Board &);

    bool is_occupied(const Position& pos) const;
    bool is_black_piece(const Position& pos) const;
    bool is_dame_piece(const Position& pos) const;
    void promote_piece(const Position& pos);
    void move_piece(const Position& from, const Position& to);
    void remove_piece(const Position& pos);
    int get_piece_count(PieceColor color) const;
    int get_piece_count() const;

    // Resets the board to the initial state
    void reset();

private:
    Pieces pieces;
};
