#include <iostream>
#include <format>
#include <assert.h>

#include "Position.h"
#include "Board.h"
#include "Piece.h"
#include "Analyzer.h"
#include "main.h"

// START of Helper functions
constexpr std::string_view piece_symbol(bool is_black, bool is_dame) noexcept {
    return is_black
               ? (is_dame ? "□" : "○")
               : (is_dame ? "■" : "●");
}

void print_board(const Board& board) {
    std::cout << "   ";
    for (char col : std::views::iota('A', 'I')) {
        std::cout << col << " ";
    }
    std::cout << "\n";
    
    for (int i : std::views::iota(0, 8)) {
        std::cout << " " << i + 1 << " ";
        for (int j : std::views::iota(0, 8)) {
            std::string_view symbol = " ";
            if ((i + j) % 2 == 0) {
                symbol = ".";
            } else if (board.is_occupied(Position{j, i})) {
                symbol = piece_symbol(board.is_black_piece(Position{j, i}), board.is_dame_piece(Position{j, i}));
            } else {
                symbol = " ";
            }
            std::cout << symbol << " ";
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}


[[deprecated("Use index-based iteration with get_position() and get_capture_pieces() instead")]]
void print_valid_positions(const CaptureSequences& sequences) {
    std::cout << "Valid Capture Sequences:\n";
    for (const auto& sequence : sequences) {
        std::cout << "  - ";
        for (const auto& pos : sequence) {
            std::cout << pos.to_string() << " ";
        }
        std::cout << "(" << sequence.size() << ")";
        std::cout << "\n";
    }
}
// END of Helper functions

void test_da_capture_sequences() {
    constexpr Position focus{"D5"};
    const Pieces pieces = {
        {focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
        {Position{"C2"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"C4"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"C6"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"E2"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"E4"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"E6"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"G2"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"G4"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"G6"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
    };
    Board board(pieces);
    std::cout << std::format("Focus Position: {}\n", focus.to_string());
    print_board(board);

    Analyzer analyzer(board);
    const auto &optional_moves = analyzer.find_valid_moves(focus);
    const auto &sequence_count = optional_moves.size();
    std::cout << std::format("Valid Positions count: {}\n", sequence_count);

    // Assert the correct total count - should be 22 unique sequences after deduplication
    assert(sequence_count == 22);
    std::cout << std::format("✓ Valid positions count assertion passed: {} unique sequences found\n", sequence_count);

    // Verify the actual path length distribution using index-based access
    std::map<size_t, int> path_length_counts;
    for (std::size_t i = 0; i < sequence_count; ++i) {
        const auto &captured_pieces = optional_moves.get_capture_pieces(i);
        // Path length = 2 * number_of_captures (captured piece + landing position for each capture)
        size_t path_length = captured_pieces.size() * 2;
        path_length_counts[path_length]++;
    }

    std::cout << "Actual path length distribution:\n";
    for (const auto &[length, count] : path_length_counts) {
        std::cout << std::format("  - {} sequences of length {}\n", count, length);
    }

    // Assert the correct path length distribution (based on deduplicated analysis)
    assert(path_length_counts[6] == 2);   // 2 short sequences (3 captures each)
    assert(path_length_counts[12] == 6);  // 6 medium-short sequences (6 captures each)
    assert(path_length_counts[14] == 10); // 10 medium sequences (7 captures each)
    assert(path_length_counts[16] == 2);  // 2 medium-long sequences (8 captures each)
    assert(path_length_counts[18] == 2);  // 2 maximal sequences (9 captures each)

    std::cout << "✓ Path length distribution (after deduplication):\n";
    std::cout << "  - 2 sequences of length 6 (3 captures each)\n";
    std::cout << "  - 6 sequences of length 12 (6 captures each)\n";
    std::cout << "  - 10 sequences of length 14 (7 captures each)\n";
    std::cout << "  - 2 sequences of length 16 (8 captures each)\n";
    std::cout << "  - 2 sequences of length 18 (9 captures each)\n";

    std::cout << "All assertions passed! The algorithm correctly finds all 22 unique capture sequences.\n";
    
    // Print sequences using index-based access
    std::cout << "Valid Capture Sequences:\n";
    for (std::size_t i = 0; i < sequence_count; ++i) {
        const auto &position = optional_moves.get_position(i);
        const auto &captured_pieces = optional_moves.get_capture_pieces(i);
        std::cout << "  - Target: " << position.to_string() << " Captures: ";
        for (const auto &piece : captured_pieces) {
            std::cout << piece.to_string() << " ";
        }
        std::cout << "(" << captured_pieces.size() << ")\n";
    }

    // Verification: Check that actual sequences match expected sequences
    std::cout << "\n=== VERIFICATION ===\n";
    
    // Expected sequences as provided for verification
    const std::vector<std::pair<std::string, std::vector<std::string>>> expected_sequences = {
        {"B7", {"C4", "C2", "E2", "G2", "E4", "C6"}},
        {"B7", {"C4", "C2", "E2", "G2", "E4", "E6", "G6", "G4", "C6"}},
        {"H1", {"C4", "C2", "E2", "E4", "C6", "G2"}},
        {"H1", {"C4", "C2", "E2", "E4", "E6", "G6", "G4", "C6", "G2"}},
        {"B3", {"C4", "E6", "G6", "G4", "E2", "C2"}},
        {"B7", {"C4", "E6", "G6", "G4", "G2", "E4", "C6"}},
        {"H1", {"C4", "E6", "G6", "G4", "E4", "C6", "G2"}},
        {"B7", {"E4", "E2", "G4", "G6", "E6", "G2", "C6"}},
        {"D1", {"E4", "E2", "G4", "G6", "E6", "C4", "C2"}},
        {"H1", {"E4", "E2", "G4", "G6", "E6", "C6", "G2"}},
        {"B7", {"E4", "G2", "C6"}},
        {"B7", {"E4", "G4", "E2", "C2", "C4", "G2", "C6"}},
        {"H1", {"E4", "G4", "E2", "C2", "C4", "C6", "G2"}},
        {"H5", {"E4", "G4", "E2", "C2", "C4", "E6", "G6"}},
        {"B7", {"E4", "G4", "G6", "E6", "G2", "C6"}},
        {"H1", {"E4", "G4", "G6", "E6", "C6", "G2"}},
        {"H1", {"E4", "C6", "G2"}},
        {"D1", {"C6", "E4", "E2", "G4", "G6", "E6", "C4", "C2"}},
        {"H5", {"C6", "E4", "G4", "E2", "C2", "C4", "E6", "G6"}},
        {"B7", {"E6", "C4", "C2", "E2", "G2", "E4", "C6"}},
        {"H1", {"E6", "C4", "C2", "E2", "E4", "C6", "G2"}},
        {"F7", {"E6", "C4", "C2", "E2", "G4", "G6"}}
    };

    // Check if all expected sequences are found
    bool all_verified = true;
    for (const auto& [expected_target, expected_captures] : expected_sequences) {
        bool found = false;
        for (std::size_t i = 0; i < sequence_count && !found; ++i) {
            const auto &actual_position = optional_moves.get_position(i);
            const auto &actual_captured_pieces = optional_moves.get_capture_pieces(i);
            
            if (actual_position.to_string() == expected_target && 
                actual_captured_pieces.size() == expected_captures.size()) {
                
                // Check if captures match (order matters)
                bool captures_match = true;
                for (std::size_t j = 0; j < expected_captures.size(); ++j) {
                    if (actual_captured_pieces[j].to_string() != expected_captures[j]) {
                        captures_match = false;
                        break;
                    }
                }
                
                if (captures_match) {
                    found = true;
                }
            }
        }
        
        if (!found) {
            std::cout << "❌ MISSING: Target " << expected_target << " with captures: ";
            for (const auto& cap : expected_captures) {
                std::cout << cap << " ";
            }
            std::cout << "(" << expected_captures.size() << ")\n";
            all_verified = false;
        }
    }

    // Check if there are any unexpected sequences
    for (std::size_t i = 0; i < sequence_count; ++i) {
        const auto &actual_position = optional_moves.get_position(i);
        const auto &actual_captured_pieces = optional_moves.get_capture_pieces(i);
        
        bool found_in_expected = false;
        for (const auto& [expected_target, expected_captures] : expected_sequences) {
            if (actual_position.to_string() == expected_target && 
                actual_captured_pieces.size() == expected_captures.size()) {
                
                bool captures_match = true;
                for (std::size_t j = 0; j < expected_captures.size(); ++j) {
                    if (actual_captured_pieces[j].to_string() != expected_captures[j]) {
                        captures_match = false;
                        break;
                    }
                }
                
                if (captures_match) {
                    found_in_expected = true;
                    break;
                }
            }
        }
        
        if (!found_in_expected) {
            std::cout << "❌ UNEXPECTED: Target " << actual_position.to_string() << " with captures: ";
            for (const auto& piece : actual_captured_pieces) {
                std::cout << piece.to_string() << " ";
            }
            std::cout << "(" << actual_captured_pieces.size() << ")\n";
            all_verified = false;
        }
    }

    if (all_verified) {
        std::cout << "✅ VERIFICATION PASSED: All 22 sequences match expected results exactly!\n";
    } else {
        std::cout << "❌ VERIFICATION FAILED: Some sequences don't match expected results.\n";
    }

    assert(all_verified);
    std::cout << "✓ Sequence verification assertion passed!\n";
}

void test_da_valid_moves()
{
    constexpr Position focus{"D5"};
    const Pieces pieces = {
        {focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
        {Position{"B3"}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
    };
    Board board(pieces);
    std::cout << std::format("Focus Position: {}\n", focus.to_string());
    print_board(board);

    Analyzer analyzer(board);
    const auto &optional_moves = analyzer.find_valid_moves(focus);
    const auto &size = optional_moves.size();
    std::cout << std::format("Valid Positions count: {}\n", size);

    for (std::size_t i = 0; i < size; ++i) {
        const auto &position = optional_moves.get_position(i);
        std::cout << position.to_string() << " ";
        
        if (optional_moves.has_captured()) {
            const auto &captured_pieces = optional_moves.get_capture_pieces(i);
            if (!captured_pieces.empty()) {
                std::cout << "(captures: ";
                for (const auto &piece : captured_pieces) {
                    std::cout << piece.to_string() << " ";
                }
                std::cout << ")";
            }
        }
    }
    std::cout << std::endl;
}

void test_pa_valid_moves()
{
    constexpr Position focus{"E8"};
    const Pieces pieces = {
        {focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}}
    };
    Board board(pieces);
    std::cout << std::format("Focus Position: {}\n", focus.to_string());
    print_board(board);

    Analyzer analyzer(board);
    const auto &optional_moves = analyzer.find_valid_moves(focus);
    const auto &size = optional_moves.size();
    std::cout << std::format("Valid Positions count: {}\n", size);

    for (std::size_t i = 0; i < size; ++i) {
        const auto &position = optional_moves.get_position(i);
        std::cout << position.to_string() << " ";
        
        if (optional_moves.has_captured()) {
            const auto &captured_pieces = optional_moves.get_capture_pieces(i);
            if (!captured_pieces.empty()) {
                std::cout << "(captures: ";
                for (const auto &piece : captured_pieces) {
                    std::cout << piece.to_string() << " ";
                }
                std::cout << ")";
            }
        }
    }
    std::cout << std::endl;
}

void test_pa_capture_sequences()
{
    constexpr Position focus{"E8"};
    const Pieces pieces = {
        {focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
        {Position{"B5"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"B3"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"D3"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"D5"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"D7"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"F3"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"F5"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        {Position{"F7"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
    };
    Board board(pieces);
    std::cout << std::format("Focus Position: {}\n", focus.to_string());
    print_board(board);

    Analyzer analyzer(board);
    const auto &optional_moves = analyzer.find_valid_moves(focus);
    const auto &sequence_count = optional_moves.size();
    std::cout << std::format("Valid Capture Sequences count: {}\n", sequence_count);

    for (std::size_t i = 0; i < sequence_count; ++i) {
        const auto &position = optional_moves.get_position(i);
        const auto &captured_pieces = optional_moves.get_capture_pieces(i);
        std::cout << "- Target " << position.to_string() << " Captures: ";
        for (const auto &piece : captured_pieces) {
            std::cout << piece.to_string() << " ";
        }
        std::cout << "(" << captured_pieces.size() << ")\n";
    }

    std::cout << std::endl;
}


int main() {
    test_da_capture_sequences();
    test_da_valid_moves();

    test_pa_capture_sequences();
    test_pa_valid_moves();
    return 0;
}
