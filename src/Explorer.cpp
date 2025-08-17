#include "Explorer.h"
#include <stdexcept>
#include <array>
#include <algorithm>
#include <ranges>
#include <concepts>
#include <span>
#include <utility>

// Static member definition
constexpr std::array<AnalyzerDirectionDelta, 4> Explorer::dir_deltas;

Legals Explorer::find_valid_moves(const Position& from) const {
    if (!board.is_occupied(from)) [[unlikely]] {
        throw std::invalid_argument("No piece at position");
    }

    std::unordered_map<SequenceKey, CaptureSequence, SequenceKeyHash> unique_sequences;
    unique_sequences.reserve(64);
    const CaptureSequence empty_sequence;
    find_capture_sequences_recursive(board, from, 0ull, empty_sequence, unique_sequences);

    CaptureSequences capture_sequences;
    for (auto& kv : unique_sequences) { capture_sequences.insert(std::move(kv.second)); }

    if (!capture_sequences.empty()) { return Legals(capture_sequences); }

    // No captures available, find regular moves
    const auto regular_positions = find_regular_moves(from);
    return Legals(regular_positions);
}


const std::vector<AnalyzerDirectionDelta>& Explorer::get_valid_directions(const Board& board,
                                                                          const Position& pos) noexcept {
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


std::optional<AnalyzerCaptureMove> Explorer::find_capture_in_direction(const Board& board, const Position& pos,
                                                                       const AnalyzerDirectionDelta& delta,
                                                                       bool is_dame) noexcept {
    const bool player_is_black = board.is_black_piece(pos);
    if (is_dame) {
        // Dame logic: search along the direction for an opponent piece to capture
        for (const auto step : std::views::iota(1u, 8u)) [[likely]] { // C++20 ranges view
            const auto [current_col, current_row] =
                std::pair{pos.x() + delta.col * static_cast<int>(step), pos.y() + delta.row * static_cast<int>(step)};

            if (!Position::is_valid(current_col, current_row)) {
                break; // Reached board boundary
            }

            const Position current_pos(current_col, current_row);
            if (!board.is_occupied(current_pos)) {
                continue; // Empty square, keep searching
            }

            // Found a piece - check if it's an opponent piece
            const bool is_opponent = board.is_black_piece(current_pos) != player_is_black;
            if (!is_opponent) {
                break; // Friendly piece blocks the path
            }

            // Found opponent piece - check if we can land beyond it
            const auto [landing_col, landing_row] = std::pair{current_col + delta.col, current_row + delta.row};
            if (Position::is_valid(landing_col, landing_row)) {
                const Position landing_pos(landing_col, landing_row);
                if (!board.is_occupied(landing_pos)) {
                    return AnalyzerCaptureMove{.captured_piece = current_pos, .landing_position = landing_pos};
                }
            }
            break;
        }
    } else {
        // Pion logic: check the adjacent square in the given direction
        const auto [adjacent_col, adjacent_row] = std::pair{pos.x() + delta.col, pos.y() + delta.row};
        if (!Position::is_valid(adjacent_col, adjacent_row)){
            return std::nullopt; // Can't capture outside board
        }

        const Position adjacent_pos(adjacent_col, adjacent_row);
        if (!board.is_occupied(adjacent_pos)) {
            return std::nullopt; // No piece to capture
        }

        // Check if it's an opponent piece
        const bool is_opponent = board.is_black_piece(adjacent_pos) != player_is_black;
        if (!is_opponent) {
            return std::nullopt; // Can't capture friendly piece
        }

        // Check if we can land beyond the captured piece
        const auto [landing_col, landing_row] = std::pair{adjacent_col + delta.col, adjacent_row + delta.row};
        if (!Position::is_valid(landing_col, landing_row)) {
            return std::nullopt; // Landing position is outside board
        }

        // Create landing position
        const Position landing_pos(landing_col, landing_row);
        if (board.is_occupied(landing_pos)) {
            return std::nullopt; // Landing square is occupied
        }

        return AnalyzerCaptureMove{.captured_piece = adjacent_pos, .landing_position = landing_pos};
    }

    return std::nullopt;
}


void Explorer::find_capture_sequences_recursive(
    Board board, const Position& current_pos, std::uint64_t captured_mask, CaptureSequence current_sequence,
    std::unordered_map<SequenceKey, CaptureSequence, SequenceKeyHash>& unique_sequences) const {

    std::vector<AnalyzerCaptureMove> valid_captures;
    valid_captures.reserve(4);
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
            SequenceKey key{.captured_mask = captured_mask, .final_pos = current_pos};
            unique_sequences.try_emplace(key, std::move(current_sequence));
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
        new_mask |= (1ull << captured_piece.hash());

        // Sequence bookkeeping
        new_sequence.emplace_back(captured_piece);
        new_sequence.emplace_back(landing_position);

        // Recurse
        find_capture_sequences_recursive(std::move(new_board), landing_position, new_mask, std::move(new_sequence),
                                         unique_sequences);
    }
}


Positions Explorer::find_regular_moves(const Position& from) const {
    const auto& valid_directions = get_valid_directions(board, from);
    const bool is_dame = board.is_dame_piece(from);

    Positions positions;
    for (const auto& delta : valid_directions) {
        if (is_dame) {
            // Dame can move multiple squares in a direction until blocked
            // Use ranges::iota for modern iteration
            for (const auto step : std::views::iota(1u, 8u)) {
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
