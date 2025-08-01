#pragma once
#include <memory>
#include <map>
#include <concepts>
#include <ranges>

#include "Position.h"
#include "Piece.h"

using Pieces = std::map<Position, PieceInfo>;

class Board {
public:
    Board() = default;
    ~Board() = default;
    Board(const Board&) = default;
    Board(Board&&) noexcept = default;
    explicit Board(const Pieces& pieces) : pieces_(pieces) {}
    explicit Board(Pieces&& pieces) noexcept : pieces_(std::move(pieces)) {}

    [[nodiscard]] static constexpr bool is_valid_position(const Position& pos) noexcept;
    [[nodiscard]] static Board from_hash(std::size_t hash_value);
    [[nodiscard]] static Board copy(const Board& other) { return Board{other}; }

    [[nodiscard]] std::size_t hash() const noexcept;
    Board& operator=(const Board&) = default;
    Board& operator=(Board&&) noexcept = default;

    [[nodiscard]] bool is_occupied(const Position& pos) const noexcept;
    [[nodiscard]] bool is_black_piece(const Position& pos) const;
    [[nodiscard]] bool is_dame_piece(const Position& pos) const;
    
    void promote_piece(const Position& pos);
    void move_piece(const Position& from, const Position& to);
    void remove_piece(const Position& pos) noexcept;
    
    [[nodiscard]] int get_piece_count(PieceColor color) const noexcept;
    [[nodiscard]] int get_piece_count() const noexcept;

    // Resets the board to the initial state
    void reset() noexcept;

    // Modern C++20 range access
    [[nodiscard]] auto pieces_view() const noexcept -> const Pieces& { return pieces_; }
    [[nodiscard]] auto begin() const noexcept { return pieces_.begin(); }
    [[nodiscard]] auto end() const noexcept { return pieces_.end(); }

private:
    Pieces pieces_;
};
