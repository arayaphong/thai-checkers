#pragma once
#include <memory>
#include <unordered_map>
#include <concepts>
#include <ranges>
#include <optional>
#include <vector>
#include <set>
#include <cstdint>
#include <compare>
#include <utility>

#include "Position.h"
#include "Piece.h"

using Pieces = std::unordered_map<Position, PieceInfo>;

enum class AnalyzerDirection : std::uint8_t { 
    NW = 0,
    NE = 1,
    SW = 2,
    SE = 3
};

template<typename T>
concept ValidAnalyzerDirection = std::same_as<T, AnalyzerDirection>;

struct AnalyzerDirectionDelta {
    std::int8_t row{};
    std::int8_t col{};
    [[nodiscard]] constexpr auto operator<=>(const AnalyzerDirectionDelta&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const AnalyzerDirectionDelta&) const noexcept = default;
    [[nodiscard]] constexpr bool is_diagonal() const noexcept { return (row == col) || (row == -col); }
    [[nodiscard]] constexpr bool is_forward(bool is_black_piece) const noexcept { return is_black_piece ? (row > 0) : (row < 0); }
};

struct AnalyzerCaptureMove {
    Position captured_piece{};
    Position landing_position{};
    [[nodiscard]] constexpr auto operator<=>(const AnalyzerCaptureMove&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const AnalyzerCaptureMove&) const noexcept = default;
    [[nodiscard]] constexpr bool is_valid() const noexcept { return captured_piece != landing_position; }
};

class Board {
private:
    static constexpr std::size_t PIECES_RESERVE_SIZE = 32;
    // 32 playable squares on an 8x8 board (only dark squares)
    // Bit i corresponds to Position::from_index(i)
    std::uint32_t occ_bits_{};   // occupied squares mask
    std::uint32_t black_bits_{}; // 1 => black piece at that index
    std::uint32_t dame_bits_{};  // 1 => dame at that index

public:
    Board() = default;
    ~Board() = default;
    Board(const Board&) = default;
    Board(Board&&) noexcept = default;
    explicit Board(const Pieces& pieces) { *this = from_pieces(pieces); }
    explicit Board(Pieces&& pieces) noexcept { *this = from_pieces(pieces); }

    [[nodiscard]] static Board setup() noexcept;
    
    [[nodiscard]] static constexpr bool is_valid_position(const Position& pos) noexcept {
        return Position::is_valid(pos.x(), pos.y()) && ((pos.x() + pos.y()) % 2 != 0);
    }
    [[nodiscard]] static Board copy(const Board& other) { return Board{other}; }

    [[nodiscard]] std::size_t hash() const noexcept;
    Board& operator=(const Board&) = default;
    Board& operator=(Board&&) noexcept = default;
    
    // Comparison operators for use in containers like std::map
    [[nodiscard]] bool operator==(const Board& other) const noexcept = default;

    [[nodiscard]] bool is_occupied(const Position& pos) const noexcept;
    [[nodiscard]] bool is_black_piece(const Position& pos) const noexcept;
    [[nodiscard]] bool is_dame_piece(const Position& pos) const noexcept;
    
    // Pass Position by const reference to avoid copying
    void promote_piece(const Position& pos) noexcept;
    void move_piece(const Position& from, const Position& to) noexcept;
    void remove_piece(const Position& pos) noexcept;

    [[nodiscard]] Pieces get_pieces(PieceColor color) const noexcept;

private:
    // Internal helpers
    [[nodiscard]] static constexpr std::uint32_t bit(std::size_t idx) noexcept {
        return static_cast<std::uint32_t>(1u) << static_cast<unsigned>(idx);
    }
    [[nodiscard]] static Board from_pieces(const Pieces& pieces) noexcept {
        Board b;
        for (const auto& [pos, info] : pieces) {
            const auto i = pos.hash();
            const auto m = bit(i);
            b.occ_bits_ |= m;
            if (info.color == PieceColor::BLACK) b.black_bits_ |= m; else b.black_bits_ &= ~m;
            if (info.type == PieceType::DAME)  b.dame_bits_  |= m; else b.dame_bits_  &= ~m;
        }
        return b;
    }
};

// C++20 improved std::hash specialization
namespace std {
    template <>
    struct hash<Board> {
        [[nodiscard]] std::size_t operator()(const Board& board) const noexcept {
            return board.hash();
        }
    };
}
