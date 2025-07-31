#include "PionAnalyzer.h"
#include <stdexcept>
#include <array>
#include <algorithm>
#include <ranges>

void PionAnalyzer::validate_pion_position(const Board& board, const Position& pos) {
    if (!Position::is_valid(pos.x, pos.y)) [[unlikely]] {
        throw std::invalid_argument("Invalid position coordinates");
    }

    if (!board.is_occupied(pos)) [[unlikely]] {
        throw std::invalid_argument("Position is not occupied");
    }

    if (board.is_dame_piece(pos)) [[unlikely]] {
        throw std::invalid_argument("Position contains a dame piece, not a pion piece");
    }
}

std::array<PionDirectionDelta, 2> PionAnalyzer::get_forward_directions(
    const Board& board, const Position& pos) {
    
    const bool is_black = board.is_black_piece(pos);
    
    if (is_black) {
        // Black pieces move downward (increasing row numbers)
        return {{
            {1, -1}, // Down-left (SW)
            {1,  1}  // Down-right (SE)
        }};
    } else {
        // White pieces move upward (decreasing row numbers)
        return {{
            {-1, -1}, // Up-left (NW)
            {-1,  1}  // Up-right (NE)
        }};
    }
}

std::optional<Move> PionAnalyzer::find_capture_in_direction(
    const Board& board, const Position& pos, const PionDirectionDelta& delta) {
    
    const auto player_color = board.is_black_piece(pos) ? PieceColor::BLACK : PieceColor::WHITE;
    
    // Check the adjacent square in the given direction
    const int adjacent_col = pos.x + delta.col;
    const int adjacent_row = pos.y + delta.row;
    
    if (!Position::is_valid(adjacent_col, adjacent_row)) {
        return std::nullopt; // Can't capture outside board
    }
    
    const Position adjacent_pos(adjacent_col, adjacent_row);
    
    if (!board.is_occupied(adjacent_pos)) {
        return std::nullopt; // No piece to capture
    }
    
    // Check if it's an opponent piece
    const bool is_opponent = board.is_black_piece(adjacent_pos) != (player_color == PieceColor::BLACK);
    if (!is_opponent) {
        return std::nullopt; // Can't capture friendly piece
    }
    
    // Check if we can land beyond the captured piece
    const int landing_row = adjacent_row + delta.row;
    const int landing_col = adjacent_col + delta.col;
    
    if (!Position::is_valid(landing_col, landing_row)) {
        return std::nullopt; // Can't land outside board
    }
    
    const Position landing_pos(landing_col, landing_row);
    if (board.is_occupied(landing_pos)) {
        return std::nullopt; // Landing square is occupied
    }
    
    return Move::make_capture(0, pos, landing_pos, adjacent_pos);
}

void PionAnalyzer::find_capture_sequences_recursive(
    Board board, // Copy by value for simulation
    const Position& current_pos,
    std::set<Position> captured_pieces,
    CaptureSequence current_sequence,
    CaptureSequences& all_sequences) const {
    
    std::vector<PionCaptureMove> valid_captures;
    const auto forward_directions = get_forward_directions(board, current_pos);
    
    // Find all valid captures from current position (only forward directions)
    for (const auto& delta : forward_directions) {
        auto capture_move = find_capture_in_direction(board, current_pos, delta);
        
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

CaptureSequences PionAnalyzer::find_capture_sequences(const Position& from) const {
    validate_pion_position(board, from);
    
    CaptureSequences all_sequences;
    CaptureSequence empty_sequence;
    std::set<Position> no_captured_pieces;
    
    find_capture_sequences_recursive(board, from, no_captured_pieces, empty_sequence, all_sequences);
    return deduplicate_equivalent_sequences(all_sequences);
}

CaptureSequences PionAnalyzer::deduplicate_equivalent_sequences(const CaptureSequences& all_sequences) {
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

bool PionAnalyzer::has_captures(const Position& from) const {
    validate_pion_position(board, from);
    
    const auto forward_directions = get_forward_directions(board, from);
    
    return std::ranges::any_of(forward_directions, [&](const auto& delta) {
        return find_capture_in_direction(board, from, delta).has_value();
    });
}

Positions PionAnalyzer::find_regular_moves(const Position& from) const {
    validate_pion_position(board, from);
    
    Positions valid_positions;
    const auto forward_directions = get_forward_directions(board, from);
    
    // Check each forward direction for a single-step move
    for (const auto& delta : forward_directions) {
        const int target_row = from.y + delta.row;
        const int target_col = from.x + delta.col;

        if (!Position::is_valid(target_col, target_row)) {
            continue; // Position is outside board
        }

        const Position target_pos(target_col, target_row);

        if (!board.is_occupied(target_pos)) {
            valid_positions.push_back(target_pos);
        }
    }

    return valid_positions;
}

Options PionAnalyzer::find_valid_moves(const Position& from) const {
    if (has_captures(from)) {
        return Options(find_capture_sequences(from));
    } else {
        return Options(find_regular_moves(from));
    }
}
