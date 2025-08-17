#pragma once

#include "Piece.h"
#include "Position.h"
#include <compare>
#include <concepts>
#include <cstdint>
#include <optional>
#include <ranges>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

using Pieces = std::unordered_map<Position, PieceInfo>;

enum class AnalyzerDirection : std::uint8_t { NW = 0, NE = 1, SW = 2, SE = 3 };

template <typename T>
concept ValidAnalyzerDirection = std::same_as<T, AnalyzerDirection>;

struct BoardConstants {
    static constexpr int kBoardDimension = 8;
    static constexpr int kInitialPieceRows = 2;
    static constexpr int kPiecesPerRow = 4;
    static constexpr int kWhiteStartingRow = 6;
    static constexpr int kBoardSquares = 32;
    static constexpr int kMaxPiecesPerSide = 16;
    static constexpr int kBitsetSize = 64;
    static constexpr int kPiecesReserveSize = 12;
};

struct AnalyzerDirectionDelta {
    std::int8_t row{};
    std::int8_t col{};
    [[nodiscard]] constexpr auto operator<=>(const AnalyzerDirectionDelta&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const AnalyzerDirectionDelta&) const noexcept = default;
    [[nodiscard]] constexpr bool is_diagonal() const noexcept { return (row == col) || (row == -col); }
    [[nodiscard]] constexpr bool is_forward(bool is_black_piece) const noexcept {
        return is_black_piece ? (row > 0) : (row < 0);
    }
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
    // 32 playable squares on an 8x8 board (only dark squares)
    // Bit i corresponds to Position::from_index(i)
    std::uint32_t occ_bits_{};   // occupied squares mask
    std::uint32_t black_bits_{}; // 1 => black piece at that index
    std::uint32_t dame_bits_{};  // 1 => dame at that index
    // Internal helpers
    [[nodiscard]] static constexpr std::uint32_t bit(std::size_t idx) noexcept {
        return static_cast<std::uint32_t>(1u) << static_cast<unsigned>(idx);
    }
    [[nodiscard]] static Board from_pieces(const Pieces& pieces);
    [[nodiscard]] static auto setup_pieces() -> Board;

  public:
    Board() = default;
    ~Board() = default;
    Board(const Board&) = default;
    Board(Board&&) noexcept = default;
    explicit Board(const Pieces& pieces) { *this = from_pieces(pieces); }
    explicit Board(Pieces&& pieces) { *this = from_pieces(pieces); }

    [[nodiscard]] static Board setup();

    [[nodiscard]] static constexpr bool is_valid_position(const Position& pos) noexcept {
        return Position::is_valid(pos.x(), pos.y()) && ((pos.x() + pos.y()) % 2 != 0);
    }
    [[nodiscard]] static Board copy(const Board& other) { return Board{other}; }
    [[nodiscard]] static Board from_hash(std::size_t hash);

    [[nodiscard]] std::size_t hash() const noexcept;
    [[nodiscard]] operator std::size_t() const noexcept { return hash(); }
    Board& operator=(const Board&) = default;
    Board& operator=(Board&&) noexcept = default;

    // Comparison operators for use in containers like std::map
    [[nodiscard]] bool operator==(const Board& other) const noexcept = default;

    [[nodiscard]] bool is_occupied(const Position& pos) const noexcept;
    [[nodiscard]] bool is_black_piece(const Position& pos) const noexcept;
    [[nodiscard]] bool is_dame_piece(const Position& pos) const noexcept;

    // Pass Position by const reference to avoid copying
    void promote_piece(const Position& pos) noexcept;
    void move_piece(const Position& from_pos, const Position& to_pos) noexcept;
    void remove_piece(const Position& pos) noexcept;

    [[nodiscard]] Pieces get_pieces(PieceColor color) const;
};

// C++20 improved std::hash specialization
namespace std {
template <> struct hash<Board> {
    [[nodiscard]] std::size_t operator()(const Board& board) const noexcept { return board.hash(); }
};
} // namespace std
