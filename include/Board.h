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
    static constexpr std::size_t PIECES_RESERVE_SIZE = 16;

public:
    Board() { pieces_.reserve(PIECES_RESERVE_SIZE); }
    ~Board() = default;
    Board(const Board&) = default;
    Board(Board&&) noexcept = default;
    explicit Board(const Pieces& pieces) : pieces_(pieces) { pieces_.reserve(PIECES_RESERVE_SIZE); }
    explicit Board(Pieces&& pieces) noexcept : pieces_(std::move(pieces)) { pieces_.reserve(PIECES_RESERVE_SIZE); }

    [[nodiscard]] static Board setup() noexcept;
    
    [[nodiscard]] static constexpr bool is_valid_position(const Position& pos) noexcept {
        return Position::is_valid(pos.x(), pos.y()) && ((pos.x() + pos.y()) % 2 != 0);
    }
    [[nodiscard]] static Board from_hash(std::size_t hash_value);
    [[nodiscard]] static Board copy(const Board& other) { return Board{other}; }

    [[nodiscard]] std::size_t hash() const noexcept;
    Board& operator=(const Board&) = default;
    Board& operator=(Board&&) noexcept = default;
    
    // Comparison operators for use in containers like std::map
    [[nodiscard]] bool operator==(const Board& other) const noexcept = default;

    [[nodiscard]] bool is_occupied(const Position& pos) const noexcept;
    [[nodiscard]] bool is_black_piece(const Position& pos) const;
    [[nodiscard]] bool is_dame_piece(const Position& pos) const;
    
    // More efficient non-throwing variants that return optional results
    [[nodiscard]] std::optional<PieceColor> get_piece_color(const Position& pos) const noexcept;
    [[nodiscard]] std::optional<PieceType> get_piece_type(const Position& pos) const noexcept;
    [[nodiscard]] std::optional<PieceInfo> get_piece_info(const Position& pos) const noexcept;
    
    // Pass Position by const reference to avoid copying
    void promote_piece(const Position& pos);
    void move_piece(const Position& from, const Position& to);
    void remove_piece(const Position& pos) noexcept;
    
    [[nodiscard]] int get_piece_count(PieceColor color) const noexcept;
    [[nodiscard]] int get_piece_count() const noexcept;

    // Return const reference to avoid copying the container
    [[nodiscard]] const Pieces& get_pieces() const noexcept { return pieces_; }
    [[nodiscard]] Pieces get_pieces(PieceColor color) const noexcept;

    // Resets the board to the initial state
    void reset() noexcept;

    // Modern C++20 range access
    [[nodiscard]] auto pieces_view() const noexcept -> const Pieces& { return pieces_; }
    [[nodiscard]] auto begin() const noexcept { return pieces_.begin(); }
    [[nodiscard]] auto end() const noexcept { return pieces_.end(); }

private:
    Pieces pieces_;
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
