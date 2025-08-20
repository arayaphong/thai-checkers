#include "Explorer.h"
#include "Legals.h"
#include "Position.h"
#include "Board.h"
#include <optional>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <algorithm>
#include <ranges>
#include <unordered_map>
#include <utility>
#include <vector>

auto Explorer::find_valid_moves(const Position& from) const -> Legals {
    if (!board.is_occupied(from)) [[unlikely]] { throw std::invalid_argument("No piece at position"); }

    constexpr int kMaxPossibleSequences = 64;
    std::unordered_map<SequenceKey, CaptureSequence, SequenceKeyHash> unique_sequences;
    unique_sequences.reserve(kMaxPossibleSequences);
    const CaptureSequence empty_sequence;
    find_capture_sequences_recursive(board, from, 0ULL, empty_sequence, unique_sequences);

    CaptureSequences capture_sequences;
    for (auto& sequence_pair : unique_sequences) { capture_sequences.insert(std::move(sequence_pair.second)); }

    if (!capture_sequences.empty()) { return Legals(capture_sequences); }

    // No captures available, find regular moves
    const auto regular_positions = find_regular_moves(from);
    return Legals(regular_positions);
}

// Static member definition

auto Explorer::get_valid_directions(const Board& board, const Position& pos) noexcept
    -> const std::vector<AnalyzerDirectionDelta>& {
    const bool is_dame = board.is_dame_piece(pos);
    const bool is_black = board.is_black_piece(pos);

    // Use designated initializers for clarity and C++20 features
    static const auto all = std::vector<AnalyzerDirectionDelta>{
        {.row = -1, .col = -1}, // NW
        {.row = -1, .col = 1},  // NE
        {.row = 1, .col = -1},  // SW
        {.row = 1, .col = 1}    // SE
    };
    static const auto down_moves = std::vector<AnalyzerDirectionDelta>{
        {.row = 1, .col = -1}, // SW
        {.row = 1, .col = 1}   // SE
    };
    static const auto up_moves = std::vector<AnalyzerDirectionDelta>{
        {.row = -1, .col = -1}, // NW
        {.row = -1, .col = 1}   // NE
    };

    if (is_dame) { return all; }

    return is_black ? down_moves : up_moves;
}

auto Explorer::find_capture_in_direction(const Board& board, const Position& pos, const AnalyzerDirectionDelta& delta,
                                         bool is_dame) noexcept -> std::optional<AnalyzerCaptureMove> {
    try {
        const bool player_is_black = board.is_black_piece(pos);
        if (is_dame) {
            // Dame logic: search along the direction for an opponent piece to capture
            for (const auto step : std::views::iota(1U, 8U)) [[likely]] { // C++20 ranges view
                const auto [current_col, current_row] = std::pair{pos.x() + delta.col * static_cast<int>(step),
                                                                  pos.y() + delta.row * static_cast<int>(step)};

                if (!Position::is_valid(current_col, current_row)) {
                    break; // Reached board boundary
                }

                const auto current_pos = Position{current_col, current_row};
                if (board.is_occupied(current_pos)) {
                    if (board.is_black_piece(current_pos) != player_is_black) {
                        // Found an opponent piece, check if we can land behind it
                        const auto [landing_col, landing_row] =
                            std::pair{current_col + delta.col, current_row + delta.row};

                        if (Position::is_valid(landing_col, landing_row) &&
                            !board.is_occupied({landing_col, landing_row})) {
                            // Valid capture move
                            return AnalyzerCaptureMove{current_pos, Position{landing_col, landing_row}};
                        }
                    }
                    break; // Blocked by a piece
                }
            }
        } else {
            // Pion logic: check for a capture in the given direction
            const auto [capture_col, capture_row] = std::pair{pos.x() + delta.col, pos.y() + delta.row};
            const auto [landing_col, landing_row] = std::pair{pos.x() + 2 * delta.col, pos.y() + 2 * delta.row};

            if (Position::is_valid(capture_col, capture_row) && Position::is_valid(landing_col, landing_row)) {
                const auto capture_pos = Position{capture_col, capture_row};
                const auto landing_pos = Position{landing_col, landing_row};

                if (board.is_occupied(capture_pos) && board.is_black_piece(capture_pos) != player_is_black &&
                    !board.is_occupied(landing_pos)) {
                    return AnalyzerCaptureMove{capture_pos, landing_pos};
                }
            }
        }

        return std::nullopt;
    } catch (...) { return std::nullopt; }
}

// NOLINTNEXTLINE(misc-no-recursion)
void Explorer::find_capture_sequences_recursive(
    Board board, const Position& current_pos, std::uint64_t captured_mask, const CaptureSequence& current_sequence,
    std::unordered_map<SequenceKey, CaptureSequence, SequenceKeyHash>& unique_sequences) const {

    constexpr int kMaxCapturesPerTurn = 4;
    std::vector<AnalyzerCaptureMove> valid_captures;
    valid_captures.reserve(kMaxCapturesPerTurn);
    const auto& valid_directions = get_valid_directions(board, current_pos);
    const bool is_dame = board.is_dame_piece(current_pos);

    // Find all valid captures from current position using ranges
    auto capture_moves = valid_directions | std::views::transform([&](const auto& delta) {
                             return find_capture_in_direction(board, current_pos, delta, is_dame);
                         }) |
                         std::views::filter([](const auto& capture_opt) { return capture_opt.has_value(); }) |
                         std::views::transform([](const auto& capture_opt) { return capture_opt.value(); });

    // Convert to vector for easier manipulation
    std::ranges::copy(capture_moves, std::back_inserter(valid_captures));

    // If no captures are available, finalize this sequence if not empty
    if (valid_captures.empty()) {
        if (!current_sequence.empty()) {
            SequenceKey const key{.captured_mask = captured_mask, .final_pos = current_pos};
            unique_sequences.try_emplace(key, current_sequence);
        }
        return;
    }

    // Try each valid capture using range-based loop
    for (const auto& [captured_piece, landing_position] : valid_captures) [[likely]] {
        // Mutate/undo model on a copied board (we still copy once per level due to value param).
        Board new_board = board; // one copy per branch (cheap with bitboards)
        auto new_mask = captured_mask;
        auto new_sequence = current_sequence;

        // Apply capture
        new_board.remove_piece(captured_piece);
        new_board.move_piece(current_pos, landing_position);
        new_mask |= (1ULL << captured_piece.hash());

        // Sequence bookkeeping
        new_sequence.emplace_back(captured_piece);
        new_sequence.emplace_back(landing_position);

        // Recurse
        find_capture_sequences_recursive(new_board, landing_position, new_mask, new_sequence, unique_sequences);
    }
}

auto Explorer::find_regular_moves(const Position& from) const -> Positions {
    const auto& valid_directions = get_valid_directions(board, from);
    const bool is_dame = board.is_dame_piece(from);

    Positions positions;
    for (const auto& delta : valid_directions) {
        if (is_dame) {
            // Dame can move multiple squares in a direction until blocked
            // Use ranges::iota for modern iteration
            for (const auto step : std::views::iota(1U, 8U)) {
                const auto [new_col, new_row] = std::pair{from.x() + delta.col * static_cast<int>(step),
                                                          from.y() + delta.row * static_cast<int>(step)};

                if (!Position::is_valid(new_col, new_row)) {
                    break; // Reached board boundary
                }

                const Position new_pos(new_col, new_row);
                if (board.is_occupied(new_pos)) {
                    break; // Blocked by another piece
                }

                positions.emplace_back(new_pos);
            }
        } else {
            // Pion can only move one square in forward directions
            const auto [new_col, new_row] = std::pair{from.x() + delta.col, from.y() + delta.row};
            if (Position::is_valid(new_col, new_row)) {
                const Position new_pos(new_col, new_row);
                if (!board.is_occupied(new_pos)) { positions.emplace_back(new_pos); }
            }
        }
    }

    return positions;
}
