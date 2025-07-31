#include <iostream>
#include <format>
#include <assert.h>

#include "Position.h"
#include "Board.h"
#include "Piece.h"
#include "Move.h"
#include "DameAnalyzer.h"
#include "PionAnalyzer.h"
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

void print_dame_move(const Move& move) {
    std::cout << std::format("Depth: {}\n", move.depth);
    std::cout << std::format("Focus Position: {}\n", move.focus.to_string());
    std::cout << "Valid Positions: ";

    std::cout << move.target->to_string() << " ";
    std::cout << "\n";

    if (move.captured.has_value()) {
        std::cout << "Captured Position(s): ";
        const auto& captured = move.captured.value();
        std::cout << captured.to_string() << " ";
        std::cout << "\n";
    } else {
        std::cout << "No capture possible.\n";
    }
}

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

    DameAnalyzer dca(board);
    const auto &optional_moves = dca.find_valid_moves(focus);
    const auto &capture_sequences = optional_moves.get_capture_sequences();
    std::cout << std::format("Valid Positions count: {}\n", capture_sequences.size());

    // Assert the correct total count - should be 22 unique sequences after deduplication
    assert(capture_sequences.size() == 22);
    std::cout << std::format("✓ Valid positions count assertion passed: {} unique sequences found\n", capture_sequences.size());

    // Verify the actual path length distribution
    std::map<size_t, int> path_length_counts;
    for (const auto &sequence : capture_sequences) {
        path_length_counts[sequence.size()]++;
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
    print_valid_positions(capture_sequences);
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

    DameAnalyzer dca(board);
    const auto &optional_moves = dca.find_valid_moves(focus);
    const auto &valid_positions = optional_moves.get_positions();
    std::cout << std::format("Valid Positions count: {}\n", valid_positions.size());

    for (const auto &pos : valid_positions)
    {
        std::cout << pos.to_string() << " ";
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

    PionAnalyzer pa(board);
    const auto &optional_moves = pa.find_valid_moves(focus);
    const auto &valid_positions = optional_moves.get_positions();
    std::cout << std::format("Valid Positions count: {}\n", valid_positions.size());

    for (const auto &pos : valid_positions)
    {
        std::cout << pos.to_string() << " ";
    }
    std::cout << std::endl;
}

int main() {
    // test_da_capture_sequences();
    // test_da_valid_moves();

    // test_pa_valid_moves();

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

    PionAnalyzer pa(board);
    const auto &optional_moves = pa.find_valid_moves(focus);
    const auto &capture_sequences = optional_moves.get_capture_sequences();
    std::cout << std::format("Valid Capture Sequences count: {}\n", capture_sequences.size());

    for (const auto &sequence : capture_sequences) {
        std::cout << "Capture Sequence: ";
        for (const auto &pos : sequence) {
            std::cout << pos.to_string() << " ";
        }
        std::cout << "(" << sequence.size() << " captures)\n";
    }

    std::cout << std::endl;

    return 0;
}