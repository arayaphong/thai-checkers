#include "DameAnalyzer.h"
#include <stdexcept>
#include <array>
#include <algorithm>
#include <ranges>

// Static member definition
constexpr std::array<DirectionDelta, 4> DameAnalyzer::dir_deltas;

void DameAnalyzer::validate_dame_position(const Board& board, const Position& pos) {
    if (!Position::is_valid(pos.x, pos.y)) [[unlikely]] {
        throw std::invalid_argument("Invalid position coordinates");
    }

    if (!board.is_occupied(pos)) [[unlikely]] {
        throw std::invalid_argument("Position is not occupied");
    }

    if (!board.is_dame_piece(pos)) [[unlikely]] {
        throw std::invalid_argument("Position does not contain a dame piece");
    }
}

std::optional<Move> DameAnalyzer::find_capture_in_direction(
    const Board& board, const Position& pos, Direction dir) {
    
    const auto player_color = board.is_black_piece(pos) ? PieceColor::BLACK : PieceColor::WHITE;
    const auto [delta_row, delta_col] = dir_deltas[static_cast<size_t>(dir)];

    // Search along the direction for an opponent piece to capture
    for (std::uint8_t step = 1; step < 8; ++step) { // Max 8 steps on a typical board
        const int current_col = pos.x + delta_col * step;
        const int current_row = pos.y + delta_row * step;
        
        if (!Position::is_valid(current_col, current_row)) {
            break; // Reached board boundary
        }
        
        const Position current_pos(current_col, current_row);
        
        if (!board.is_occupied(current_pos)) {
            continue; // Empty square, keep searching
        }
        
        // Found a piece - check if it's an opponent piece
        const bool is_opponent = board.is_black_piece(current_pos) != (player_color == PieceColor::BLACK);
        if (!is_opponent) {
            break; // Friendly piece blocks the path
        }
        
        // Found opponent piece - check if we can land beyond it
        const int landing_row = current_row + delta_row;
        const int landing_col = current_col + delta_col;
        
        if (Position::is_valid(landing_col, landing_row)) {
            const Position landing_pos(landing_col, landing_row);
            if (!board.is_occupied(landing_pos)) {
                return Move::make_capture(0, pos, landing_pos, current_pos);
            }
        }
        break; // Can't capture (landing square occupied or invalid)
    }
    
    return std::nullopt;
}

void DameAnalyzer::find_capture_sequences_recursive(
    Board board, // Copy by value for simulation
    const Position& current_pos,
    std::set<Position> captured_pieces,
    CaptureSequence current_sequence,
    CaptureSequences& all_sequences) const {
    
    std::vector<CaptureMove> valid_captures;
    constexpr auto all_directions = std::array{Direction::NW, Direction::NE, Direction::SW, Direction::SE};
    
    // Find all valid captures from current position
    for (const auto& dir : all_directions) {
        auto capture_move = find_capture_in_direction(board, current_pos, dir);
        
        if (!capture_move.has_value()) {
            continue; // No capture in this direction
        }
        
        const auto& captured_pos = capture_move->captured.value();
        const auto& landing_pos = capture_move->target.value();
        
        // Skip if we've already captured this piece
        if (captured_pieces.contains(captured_pos)) {
            continue;
        }
        
        valid_captures.emplace_back(captured_pos, landing_pos);
    }
    
    // If no captures are available, finalize this sequence if it's not empty
    if (valid_captures.empty()) {
        if (!current_sequence.empty()) {
            all_sequences.insert(current_sequence);
        }
        return;
    }
    
    // Try each valid capture
    for (const auto& capture : valid_captures) {
        // Create new state for this capture
        auto new_board = board;
        auto new_captured = captured_pieces;
        auto new_sequence = current_sequence;
        
        // Apply the capture
        new_board.remove_piece(capture.captured_piece);
        new_board.move_piece(current_pos, capture.landing_position);
        new_captured.insert(capture.captured_piece);
        
        // Add captured position and landing position to sequence
        new_sequence.push_back(capture.captured_piece);
        new_sequence.push_back(capture.landing_position);
        
        // Recursively find more captures from the landing position
        find_capture_sequences_recursive(
            new_board, capture.landing_position, new_captured, new_sequence, all_sequences);
    }
}

CaptureSequences DameAnalyzer::find_capture_sequences(const Position& from) const {
    validate_dame_position(board, from);
    
    CaptureSequences all_sequences;
    CaptureSequence empty_sequence;
    std::set<Position> no_captured_pieces;
    
    find_capture_sequences_recursive(board, from, no_captured_pieces, empty_sequence, all_sequences);
    return deduplicate_equivalent_sequences(all_sequences);
}

CaptureSequences DameAnalyzer::deduplicate_equivalent_sequences(const CaptureSequences& all_sequences) {
    // Map from (captured_pieces, final_position) to representative sequence
    std::map<std::pair<std::set<Position>, Position>, CaptureSequence> unique_sequences;
    
    for (const auto& sequence : all_sequences) {
        if (sequence.empty()) continue;

        // Enforce that the sequence has even length (pairs of captured and landing)
        if (sequence.size() % 2 != 0) {
            throw std::logic_error("CaptureSequence format error: expected alternating captured and landing positions (even length).");
        }
        
        // Extract captured pieces (every even index) and final position (last odd index)
        std::set<Position> captured_pieces;
        Position final_position = sequence.back(); // Last position is the final landing
        
        for (size_t i = 0; i < sequence.size(); i += 2) {
            captured_pieces.insert(sequence[i]); // Captured piece positions
        }
        
        auto key = std::make_pair(captured_pieces, final_position);
        
        // Keep the first sequence we encounter for each unique combination
        if (unique_sequences.find(key) == unique_sequences.end()) {
            unique_sequences[key] = sequence;
        }
    }
    
    // Convert back to CaptureSequences
    CaptureSequences deduplicated;
    for (const auto& [key, sequence] : unique_sequences) {
        deduplicated.insert(sequence);
    }
    
    return deduplicated;
}

bool DameAnalyzer::has_captures(const Position& from) const {
    validate_dame_position(board, from);
    
    constexpr auto all_directions = std::array{Direction::NW, Direction::NE, Direction::SW, Direction::SE};
    
    return std::ranges::any_of(all_directions, [&](const auto& dir) {
        return find_capture_in_direction(board, from, dir).has_value();
    });
}

Positions DameAnalyzer::find_regular_moves(const Position& from) const {
    Positions valid_positions;

    constexpr auto all_directions = std::array{Direction::NW, Direction::NE, Direction::SW, Direction::SE};
    
    for (const auto& dir : all_directions) {
        const auto [delta_row, delta_col] = dir_deltas[static_cast<size_t>(dir)];

        // Search along the direction for valid positions
        for (std::uint8_t step = 1; step < 8; ++step) {
            const int target_row = from.y + delta_row * step;
            const int target_col = from.x + delta_col * step;

            if (!Position::is_valid(target_col, target_row)) {
                break; // Reached board boundary
            }

            const Position target_pos(target_col, target_row);

            if (board.is_occupied(target_pos)) {
                break; // Path blocked by any piece
            }

            valid_positions.push_back(target_pos);
        }
    }

    return valid_positions;
}

Options DameAnalyzer::find_valid_moves(const Position& from) const {
    if (has_captures(from)) {
        return Options(find_capture_sequences(from));
    } else {
        return Options(find_regular_moves(from));
    }
}
