#pragma once

#include <vector>
#include <set>
#include <unordered_map>
#include <optional>
#include <array>
#include <ranges>
#include <algorithm>
#include <concepts>
#include <span>
#include <stdexcept>
#include <cstdint>
#include <utility>
#include <memory>

#include "Position.h"
#include "Board.h"
#include "Piece.h"
#include "Legals.h"

/**
 * @brief Analyzes movement and capture possibilities for both Pion and Dame pieces in Thai Checkers.
 * 
 * This unified class provides functionality to find all possible capture sequences
 * and regular moves for any piece type from a given position on the board. 
 * It follows the rules of Thai Checkers where:
 * - Pions can only move diagonally forward and capture opponent pieces by jumping over them
 * - Dames can move diagonally in any direction and capture opponent pieces by jumping over them
 */
class Explorer {
private:
    const Board& board;

    // C++20: Use designated initializers for better readability
    static constexpr std::array<AnalyzerDirectionDelta, 4> dir_deltas = {{
        {.row = -1, .col = -1}, // NW
        {.row = -1, .col =  1}, // NE
        {.row =  1, .col = -1}, // SW
        {.row =  1, .col =  1}  // SE
    }};

    /**
     * @brief Gets the valid directions for a piece based on its type and color.
     * @param board The current board state.
     * @param pos The position of the piece.
     * @return Array of direction deltas for valid movement.
     */
    [[nodiscard]] static const std::vector<AnalyzerDirectionDelta>& get_valid_directions(
        const Board& board, const Position& pos) noexcept;

    /**
     * @brief Finds a capture move in a specific direction from the given position.
     * @param board The current board state.
     * @param pos The starting position.
     * @param delta The direction delta to search.
     * @param is_dame Whether the piece is a dame (affects search range).
     * @return Optional AnalyzerCaptureMove containing capture information, or nullopt if no capture is possible.
     */
    [[nodiscard]] static std::optional<AnalyzerCaptureMove> find_capture_in_direction(
        const Board& board, const Position& pos, const AnalyzerDirectionDelta& delta, bool is_dame) noexcept;

    /**
     * @brief Recursively finds all possible capture sequences starting from a position.
     * @param board Board state (copied for simulation)
     * @param current_pos Current position being analyzed
     * @param captured_pieces Set of already captured pieces
     * @param current_sequence Current capture sequence being built
     * @param all_sequences Output container for all found sequences
     */
    // Bitmask-based key (up to 32 playable squares on 8x8 checkers board)
    struct SequenceKey {
        std::uint64_t captured_mask{}; // bit i set => position with index/hash i captured
        Position final_pos{};          // landing square at end of sequence
        [[nodiscard]] constexpr auto operator<=>(const SequenceKey&) const noexcept = default;
        [[nodiscard]] constexpr bool operator==(const SequenceKey&) const noexcept = default;
    };

    // Custom hash for SequenceKey
    struct SequenceKeyHash {
        [[nodiscard]] constexpr std::size_t operator()(const SequenceKey& k) const noexcept {
            // Simple mix: final_pos hash in low bits, multiplicative scramble of mask
            // Enough for tiny key set (number of capture sequences is small)
            const auto posh = k.final_pos.hash();
            // 64-bit mix constant (from splitmix64)
            std::uint64_t x = k.captured_mask + 0x9E3779B97F4A7C15ull;
            x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
            x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
            x ^= (x >> 31);
            return static_cast<std::size_t>(x ^ (posh + 0x9e3779b97f4a7c15ull + (x<<6) + (x>>2)));
        }
    };

    void find_capture_sequences_recursive(
        Board board, // Copy by value for simulation
        const Position& current_pos,
        std::uint64_t captured_mask,
        CaptureSequence current_sequence,
        std::unordered_map<SequenceKey, CaptureSequence, SequenceKeyHash>& unique_sequences) const;

public:
    /**
     * @brief Constructs an Explorer for the given board.
     * @param board The board to analyze (stored as const reference).
     */
    explicit constexpr Explorer(const Board& board) noexcept : board(board) {}

    // C++20: Explicitly defaulted special members for better clarity
    ~Explorer() = default;
    Explorer(const Explorer&) = default;
    Explorer(Explorer&&) = default;


    /**
     * @brief Finds all valid moves for a piece at the given position.
     * @param from The starting position of the piece.
    * @return Legals containing either capture sequences or regular move positions.
     */
    [[nodiscard]] Legals find_valid_moves(const Position& from) const;

private:
    /**
     * @brief Gets all possible non-capture moves for a piece at the given position.
     * @param from The position of the piece.
    * @return A vector of possible non-capture move positions.
     */
    [[nodiscard]] Positions find_regular_moves(const Position& from) const;
};
