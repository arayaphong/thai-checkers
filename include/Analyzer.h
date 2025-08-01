#pragma once

#include <vector>
#include <set>
#include <map>
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
// Move.h include removed - not needed after refactoring to use facade pattern
#include "Options.h"
#include "AnalyzerTypes.h"

/**
 * @brief Analyzes movement and capture possibilities for both Pion and Dame pieces in Thai Checkers.
 * 
 * This unified class provides functionality to find all possible capture sequences
 * and regular moves for any piece type from a given position on the board. 
 * It follows the rules of Thai Checkers where:
 * - Pions can only move diagonally forward and capture opponent pieces by jumping over them
 * - Dames can move diagonally in any direction and capture opponent pieces by jumping over them
 */
class Analyzer {
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
     * @brief Validates that the given position contains a valid piece.
     * @param board The game board to check.
     * @param pos The position to validate.
     * @throws std::invalid_argument if validation fails.
     */
    static void validate_position(const Board& board, const Position& pos);

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
    void find_capture_sequences_recursive(
        Board board, // Copy by value for simulation
        const Position& current_pos,
        std::set<Position> captured_pieces,
        CaptureSequence current_sequence,
        CaptureSequences& all_sequences) const;

public:
    /**
     * @brief Constructs an Analyzer for the given board.
     * @param board The board to analyze (stored as const reference).
     */
    explicit constexpr Analyzer(const Board& board) noexcept : board(board) {}

    // C++20: Explicitly defaulted special members for better clarity
    ~Analyzer() = default;
    Analyzer(const Analyzer&) = default;
    Analyzer& operator=(const Analyzer&) = default;
    Analyzer(Analyzer&&) = default;
    Analyzer& operator=(Analyzer&&) = default;

    /**
     * @brief Finds all valid moves for a piece at the given position.
     * @param from The starting position of the piece.
     * @return Options containing either capture sequences or regular move positions.
     * @throws std::invalid_argument if the position is invalid or doesn't contain a piece.
     */
    [[nodiscard]] Options find_valid_moves(const Position& from) const;

private:
    /**
     * @brief Deduplicates sequences that have the same captured pieces and final position.
     * 
     * In Thai Checkers, sequences that capture the same pieces and end at the same position
     * are considered equivalent, regardless of the order of captures.
     * 
     * @param all_sequences Input sequences to deduplicate
     * @return Deduplicated set of capture sequences
     */
    [[nodiscard]] static CaptureSequences deduplicate_equivalent_sequences(const CaptureSequences& all_sequences);

    /**
     * @brief Gets all possible non-capture moves for a piece at the given position.
     * @param from The position of the piece.
     * @return A vector of possible non-capture move positions.
     * @throws std::invalid_argument if the position is invalid or doesn't contain a piece.
     */
    [[nodiscard]] Positions find_regular_moves(const Position& from) const;
};
