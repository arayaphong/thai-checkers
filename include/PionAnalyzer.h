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

#include "Position.h"
#include "Board.h"
#include "Piece.h"
#include "Move.h"
#include "Options.h"

// Type aliases for better readability
using Moves = std::vector<Move>;

enum class PionDirection : std::uint8_t { 
    FORWARD_LEFT,   // NW for black (down-left), SW for white (up-left)
    FORWARD_RIGHT   // NE for black (down-right), SE for white (up-right)
};

// Concept to ensure PionDirection is valid
template<typename T>
concept ValidPionDirection = std::same_as<T, PionDirection>;

// Semantic type for board direction deltas
struct PionDirectionDelta {
    std::int8_t row;
    std::int8_t col;
};

// Semantic type for capture move information
struct PionCaptureMove {
    Position captured_piece;
    Position landing_position;
};

/**
 * @brief Analyzes capture possibilities for Pion pieces in Thai Checkers.
 * 
 * This class provides functionality to find all possible capture sequences
 * and regular moves for a pion piece from a given position on the board. 
 * It follows the rules of Thai Checkers where pions can only move diagonally 
 * forward and capture opponent pieces by jumping over them in forward directions.
 */
class PionAnalyzer {
private:
    const Board& board;

    /**
     * @brief Validates that the given position contains a valid pion piece.
     * @param board The game board to check.
     * @param pos The position to validate.
     * @throws std::invalid_argument if validation fails.
     */
    static void validate_pion_position(const Board& board, const Position& pos);

    /**
     * @brief Gets the forward directions for a pion based on its color.
     * @param board The current board state.
     * @param pos The position of the pion.
     * @return Array of direction deltas for forward movement.
     */
    [[nodiscard]] static std::array<PionDirectionDelta, 2> get_forward_directions(
        const Board& board, const Position& pos);

    /**
     * @brief Finds a capture move in a specific forward direction from the given position.
     * @param board The current board state.
     * @param pos The starting position.
     * @param delta The direction delta to search.
     * @return Optional Move containing capture information, or nullopt if no capture is possible.
     */
    [[nodiscard]] static std::optional<Move> find_capture_in_direction(
        const Board& board, const Position& pos, const PionDirectionDelta& delta);

    /**
     * @brief Recursively finds all possible capture sequences starting from a position.
     */
    void find_capture_sequences_recursive(
        Board board, // Copy by value for simulation
        const Position& current_pos,
        std::set<Position> captured_pieces,
        CaptureSequence current_sequence,
        CaptureSequences& all_sequences) const;

public:
    /**
     * @brief Constructs a PionAnalyzer for the given board.
     * @param board The board to analyze (stored as const reference).
     */
    explicit PionAnalyzer(const Board& board) : board(board) {}

    /**
     * @brief Finds all valid moves for a pion piece at the given position.
     * @param from The starting position of the pion piece.
     * @return Options containing either capture sequences or regular move positions.
     * @throws std::invalid_argument if the position is invalid or doesn't contain a pion piece.
     */
    [[nodiscard]] Options find_valid_moves(const Position& from) const;

private:
    /**
     * @brief Deduplicates sequences that have the same captured pieces and final position.
     * 
     * In Thai Checkers, sequences that capture the same pieces and end at the same position
     * are considered equivalent, regardless of the order of captures.
     */
    [[nodiscard]] static CaptureSequences deduplicate_equivalent_sequences(const CaptureSequences& all_sequences);

    /**
     * @brief Finds all possible capture sequences for a pion piece at the given position.
     * @param from The starting position of the pion piece.
     * @return A set of capture sequences, where each sequence is a vector of positions 
     *         (alternating captured pieces and landing positions).
     * @throws std::invalid_argument if the position is invalid or doesn't contain a pion piece.
     * @deprecated Use find_valid_moves() instead and call get_capture_sequences() on the result.
     */
    [[nodiscard]] CaptureSequences find_capture_sequences(const Position& from) const;

    /**
     * @brief Checks if a pion piece at the given position has any capture moves available.
     * @param from The position to check.
     * @return true if at least one capture is possible, false otherwise.
     * @throws std::invalid_argument if the position is invalid or doesn't contain a pion piece.
     */
    [[nodiscard]] bool has_captures(const Position& from) const;

    /**
     * @brief Gets all possible non-capture moves for a pion piece at the given position.
     * @param from The position of the pion piece.
     * @return A vector of possible non-capture moves.
     * @throws std::invalid_argument if the position is invalid or doesn't contain a pion piece.
     */
    [[nodiscard]] Positions find_regular_moves(const Position& from) const;
};
