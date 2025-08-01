#include "Analyzer.h"
#include <stdexcept>
#include <array>
#include <algorithm>
#include <ranges>
#include <concepts>
#include <span>
#include <utility>

// Static member definition
constexpr std::array<AnalyzerDirectionDelta, 4> Analyzer::dir_deltas;

void Analyzer::validate_position(const Board& board, const Position& pos) {
    if (!board.is_occupied(pos)) [[unlikely]] {
        throw std::invalid_argument("Position is not occupied");
    }
}

const std::vector<AnalyzerDirectionDelta>& Analyzer::get_valid_directions(
    const Board& board, const Position& pos) noexcept {
    const bool is_dame = board.is_dame_piece(pos);
    const bool is_black = board.is_black_piece(pos);
    
    // Use designated initializers for clarity and C++20 features
    static const auto all = std::vector<AnalyzerDirectionDelta>{
        {.row = -1, .col = -1}, // NW
        {.row = -1, .col =  1}, // NE
        {.row =  1, .col = -1}, // SW
        {.row =  1, .col =  1}  // SE
    };
    static const auto down_moves = std::vector<AnalyzerDirectionDelta>{
        {.row =  1, .col = -1}, // SW
        {.row =  1, .col =  1}  // SE
    };
    static const auto up_moves = std::vector<AnalyzerDirectionDelta>{
        {.row = -1, .col = -1}, // NW
        {.row = -1, .col =  1}  // NE
    };

    if (is_dame) [[likely]] {
        return all;
    }
    
    return is_black ? down_moves : up_moves;
}

std::optional<AnalyzerCaptureMove> Analyzer::find_capture_in_direction(
    const Board& board, const Position& pos, const AnalyzerDirectionDelta& delta, bool is_dame) noexcept {
    
    const bool player_is_black = board.is_black_piece(pos);
    
    if (is_dame) [[likely]] {
        // Dame logic: search along the direction for an opponent piece to capture
        for (const auto step : std::views::iota(1u, 8u)) { // C++20 ranges view
            const auto [current_col, current_row] = std::pair{
                pos.x() + delta.col * static_cast<int>(step),
                pos.y() + delta.row * static_cast<int>(step)
            };
            
            if (!Position::is_valid(current_col, current_row)) [[unlikely]] {
                break; // Reached board boundary
            }
            
            const Position current_pos(current_col, current_row);
            
            if (!board.is_occupied(current_pos)) [[likely]] {
                continue; // Empty square, keep searching
            }
            
            // Found a piece - check if it's an opponent piece
            const bool is_opponent = board.is_black_piece(current_pos) != player_is_black;
            if (!is_opponent) [[unlikely]] {
                break; // Friendly piece blocks the path
            }
            
            // Found opponent piece - check if we can land beyond it
            const auto [landing_col, landing_row] = std::pair{
                current_col + delta.col,
                current_row + delta.row
            };
            
            if (Position::is_valid(landing_col, landing_row)) [[likely]] {
                const Position landing_pos(landing_col, landing_row);
                if (!board.is_occupied(landing_pos)) [[likely]] {
                    return AnalyzerCaptureMove{
                        .captured_piece = current_pos,
                        .landing_position = landing_pos
                    };
                }
            }
            break; // Can't capture (landing square occupied or invalid)
        }
    } else {
        // Pion logic: check the adjacent square in the given direction
        const auto [adjacent_col, adjacent_row] = std::pair{
            pos.x() + delta.col,
            pos.y() + delta.row
        };
        
        if (!Position::is_valid(adjacent_col, adjacent_row)) [[unlikely]] {
            return std::nullopt; // Can't capture outside board
        }
        
        const Position adjacent_pos(adjacent_col, adjacent_row);
        
        if (!board.is_occupied(adjacent_pos)) [[likely]] {
            return std::nullopt; // No piece to capture
        }
        
        // Check if it's an opponent piece
        const bool is_opponent = board.is_black_piece(adjacent_pos) != player_is_black;
        if (!is_opponent) [[unlikely]] {
            return std::nullopt; // Can't capture friendly piece
        }
        
        // Check if we can land beyond the captured piece
        const auto [landing_col, landing_row] = std::pair{
            adjacent_col + delta.col,
            adjacent_row + delta.row
        };
        
        if (!Position::is_valid(landing_col, landing_row)) [[unlikely]] {
            return std::nullopt; // Landing position is outside board
        }
        
        const Position landing_pos(landing_col, landing_row);
        if (board.is_occupied(landing_pos)) [[unlikely]] {
            return std::nullopt; // Landing square is occupied
        }
        
        return AnalyzerCaptureMove{
            .captured_piece = adjacent_pos,
            .landing_position = landing_pos
        };
    }
    
    return std::nullopt;
}

void Analyzer::find_capture_sequences_recursive(
    Board board, // Copy by value for simulation
    const Position& current_pos,
    std::set<Position> captured_pieces,
    CaptureSequence current_sequence,
    CaptureSequences& all_sequences) const {
    
    std::vector<AnalyzerCaptureMove> valid_captures;
    const auto& valid_directions = get_valid_directions(board, current_pos);
    const bool is_dame = board.is_dame_piece(current_pos);
    
    // Find all valid captures from current position using ranges
    auto capture_moves = valid_directions 
        | std::views::transform([&](const auto& delta) {
            return find_capture_in_direction(board, current_pos, delta, is_dame);
        })
        | std::views::filter([](const auto& capture_opt) {
            return capture_opt.has_value();
        })
        | std::views::transform([](const auto& capture_opt) {
            return capture_opt.value();
        });
    
    // Convert to vector for easier manipulation
    std::ranges::copy(capture_moves, std::back_inserter(valid_captures));
    
    // If no captures are available, finalize this sequence if it's not empty
    if (valid_captures.empty()) [[likely]] {
        if (!current_sequence.empty()) [[likely]] {
            all_sequences.insert(current_sequence);
        }
        return;
    }
    
    // Try each valid capture using range-based loop
    for (const auto& [captured_piece, landing_position] : valid_captures) {
        // Create new state for this capture using more modern initialization
        auto new_board = Board::copy(board);
        auto new_captured = captured_pieces;
        auto new_sequence = current_sequence;
        
        // Apply the capture
        new_board.remove_piece(captured_piece);
        new_board.move_piece(current_pos, landing_position);
        new_captured.insert(captured_piece);
        
        // Add captured position and landing position to sequence
        new_sequence.emplace_back(captured_piece);
        new_sequence.emplace_back(landing_position);
        
        // Recursively find more captures from the landing position
        find_capture_sequences_recursive(
            std::move(new_board), landing_position, 
            std::move(new_captured), std::move(new_sequence), all_sequences);
    }
}

Options Analyzer::find_valid_moves(const Position& from) const {
    validate_position(board, from);
    
    // Check for captures first - they are mandatory in Thai Checkers
    // Inline the capture sequence finding logic to avoid deprecated method call
    CaptureSequences all_sequences;
    const CaptureSequence empty_sequence;
    const std::set<Position> no_captured_pieces;
    
    find_capture_sequences_recursive(board, from, no_captured_pieces, empty_sequence, all_sequences);
    const auto capture_sequences = deduplicate_equivalent_sequences(all_sequences);
    
    if (!capture_sequences.empty()) {
        return Options(capture_sequences);
    }
    
    // No captures available, find regular moves
    const auto regular_positions = find_regular_moves(from);
    return Options(regular_positions);
}

CaptureSequences Analyzer::deduplicate_equivalent_sequences(const CaptureSequences& all_sequences) {
    // Map from (captured_pieces, final_position) to representative sequence
    std::map<std::pair<std::set<Position>, Position>, CaptureSequence> unique_sequences;
    
    for (const auto& sequence : all_sequences) {
        // Extract captured pieces using modern range-based approach
        std::set<Position> captured_pieces;
        
        // Use structured bindings with enumeration for clarity
        for (std::size_t i = 0; const auto& pos : sequence) {
            if (i % 2 == 0) { // Even indices are captured pieces
                captured_pieces.insert(pos);
            }
            ++i;
        }
        
        const Position final_position = sequence.back(); // Last element is the final landing position
        const auto key = std::make_pair(std::move(captured_pieces), final_position);
        
        // Use try_emplace to avoid unnecessary lookups and copies
        unique_sequences.try_emplace(key, sequence);
    }
    
    // Convert back to CaptureSequences using ranges and move semantics
    CaptureSequences result;
    auto sequence_values = unique_sequences 
        | std::views::values;
    
    for (auto&& sequence : sequence_values) {
        result.insert(std::move(sequence));
    }
    
    return result;
}

Positions Analyzer::find_regular_moves(const Position& from) const {
    validate_position(board, from);

    const auto& valid_directions = get_valid_directions(board, from);
    const bool is_dame = board.is_dame_piece(from);
    
    Positions positions;
    
    for (const auto& delta : valid_directions) {
        if (is_dame) [[likely]] {
            // Dame can move multiple squares in a direction until blocked
            // Use ranges::iota for modern iteration
            for (const auto step : std::views::iota(1u, 8u)) {
                const auto [new_col, new_row] = std::pair{
                    from.x() + delta.col * static_cast<int>(step),
                    from.y() + delta.row * static_cast<int>(step)
                };

                if (!Position::is_valid(new_col, new_row)) [[unlikely]] {
                    break; // Reached board boundary
                }

                const Position new_pos(new_col, new_row);
                if (board.is_occupied(new_pos)) [[unlikely]] {
                    break; // Blocked by another piece
                }

                positions.emplace_back(new_pos);
            }
        }
        else {
            // Pion can only move one square in forward directions
            const auto [new_col, new_row] = std::pair{
                from.x() + delta.col,
                from.y() + delta.row
            };

            if (Position::is_valid(new_col, new_row)) [[likely]] {
                const Position new_pos(new_col, new_row);
                if (!board.is_occupied(new_pos)) [[likely]] {
                    positions.emplace_back(new_pos);
                }
            }
        }
    }

    return positions;
}
