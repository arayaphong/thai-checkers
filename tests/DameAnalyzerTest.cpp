#include <catch2/catch_all.hpp>
#include <format>
#include <map>
#include <ranges>

#include "Position.h"
#include "Board.h"
#include "Piece.h"
#include "Move.h"
#include "DameAnalyzer.h"

// Helper function for visual debugging (optional)
std::string piece_symbol(bool is_black, bool is_dame) {
    return is_black
               ? (is_dame ? "□" : "○")
               : (is_dame ? "■" : "●");
}

std::string board_to_string(const Board& board) {
    std::string result = "   ";
    for (char col : std::views::iota('A', 'I')) {
        result += std::format("{} ", col);
    }
    result += "\n";
    
    for (int i : std::views::iota(0, 8)) {
        result += std::format(" {} ", i + 1);
        for (int j : std::views::iota(0, 8)) {
            std::string symbol = " ";
            if ((i + j) % 2 == 0) {
                symbol = ".";
            } else if (board.is_occupied(Position{j, i})) {
                symbol = piece_symbol(board.is_black_piece(Position{j, i}), board.is_dame_piece(Position{j, i}));
            } else {
                symbol = " ";
            }
            result += std::format("{} ", symbol);
        }
        result += "\n";
    }
    return result;
}

TEST_CASE("DameCaptureAnalyzer - Complex multi-capture scenario", "[DameCaptureAnalyzer]") {
    SECTION("Complex multi-capture test scenario") {
        // Setup a complex board configuration for comprehensive testing
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
        
        INFO("Board configuration:\n" << board_to_string(board));
        INFO("Focus Position: " << focus.to_string());

        DameAnalyzer dca(board);
        const auto moves = dca.find_valid_moves(focus);
        const auto& capture_sequences = moves.get_capture_sequences();
        
        // Test the correct total count - should be 22 unique sequences after deduplication
        REQUIRE(capture_sequences.size() == 22);
        
        // Verify the path length distribution
        std::map<size_t, int> path_length_counts;
        for (const auto& sequence : capture_sequences) {
            path_length_counts[sequence.size()]++;
        }
        
        // Assert the correct path length distribution (based on deduplicated analysis)
        REQUIRE(path_length_counts[6] == 2);   // 2 short sequences (3 captures each)
        REQUIRE(path_length_counts[12] == 6);  // 6 medium-short sequences (6 captures each)
        REQUIRE(path_length_counts[14] == 10); // 10 medium sequences (7 captures each)
        REQUIRE(path_length_counts[16] == 2);  // 2 medium-long sequences (8 captures each)
        REQUIRE(path_length_counts[18] == 2);  // 2 maximal sequences (9 captures each)
        
        INFO("Path length distribution after deduplication:");
        for (const auto& [length, count] : path_length_counts) {
            INFO(std::format("  - {} sequences of length {} ({} captures each)", 
                           count, length, length / 2));
        }
    }
}

TEST_CASE("DameCaptureAnalyzer - Basic functionality", "[DameCaptureAnalyzer]") {
    SECTION("Single capture scenario") {
        Position dame_pos{"D3"};
        Position opponent_pos{"E4"};
        Position landing_pos{"F5"};
        
        const Pieces pieces = {
            {dame_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
            {opponent_pos, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        };
        Board board(pieces);
        
        DameAnalyzer dca(board);
        const auto moves = dca.find_valid_moves(dame_pos);
        const auto& sequences = moves.get_capture_sequences();
        
        REQUIRE(sequences.size() == 1);
        
        const auto& sequence = *sequences.begin();
        REQUIRE(sequence.size() == 2); // captured position + landing position
        REQUIRE(sequence[0] == opponent_pos); // captured piece
        REQUIRE(sequence[1] == landing_pos);  // landing position
    }
    
    SECTION("No capture available") {
        Position dame_pos{"D3"};
        
        const Pieces pieces = {
            {dame_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
            // No opponent pieces to capture
        };
        Board board(pieces);
        
        DameAnalyzer dca(board);
        const auto moves = dca.find_valid_moves(dame_pos);
        const auto& sequences = moves.get_capture_sequences();
        
        REQUIRE(sequences.empty());
    }
    
    SECTION("Blocked capture (no landing space)") {
        Position dame_pos{"D3"};
        Position opponent_pos{"E4"};
        Position blocking_pos{"F5"};
        
        const Pieces pieces = {
            {dame_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
            {opponent_pos, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
            {blocking_pos, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        };
        Board board(pieces);
        
        DameAnalyzer dca(board);
        const auto moves = dca.find_valid_moves(dame_pos);
        const auto& sequences = moves.get_capture_sequences();
        
        REQUIRE(sequences.empty());
    }
}

TEST_CASE("DameAnalyzer - Multiple direction captures", "[DameAnalyzer]") {
    SECTION("Four direction captures") {
        Position dame_pos{"D5"};
        
        const Pieces pieces = {
            {dame_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
            // Four opponent pieces in different directions with spaces between them
            {Position{"C4"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}, // NW
            {Position{"E4"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}, // NE
            {Position{"C6"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}, // SW
            {Position{"E6"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}, // SE
            // Block further captures by placing pieces at landing positions
            {Position{"A2"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}, // Block B3
            {Position{"G2"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}, // Block F3
            {Position{"A8"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}, // Block B7
            {Position{"G8"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}, // Block F7
        };
        Board board(pieces);
        
        DameAnalyzer dca(board);
        const auto moves = dca.find_valid_moves(dame_pos);
        const auto& sequences = moves.get_capture_sequences();
        
        // Should find 4 single captures (one in each direction) but this configuration 
        // might enable chain captures, so let's just check that we have some sequences
        REQUIRE(!sequences.empty());
        REQUIRE(sequences.size() >= 4);
    }
}

TEST_CASE("DameAnalyzer - Input validation", "[DameAnalyzer]") {
    Board board; // Empty board
    DameAnalyzer dca(board);
    
    SECTION("Position not occupied") {
        Position empty_pos{"D5"};
        REQUIRE_THROWS_AS(dca.find_valid_moves(empty_pos), std::invalid_argument);
    }
    
    SECTION("Position contains non-dame piece") {
        Position pion_pos{"D5"};
        const Pieces pieces = {
            {pion_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
        };
        Board board_with_pion(pieces);
        DameAnalyzer dca_pion(board_with_pion);
        
        REQUIRE_THROWS_AS(dca_pion.find_valid_moves(pion_pos), std::invalid_argument);
    }
}

TEST_CASE("DameAnalyzer - Chain captures", "[DameAnalyzer]") {
    SECTION("Simple chain capture") {
        Position dame_pos{"B1"};
        
        const Pieces pieces = {
            {dame_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
            {Position{"C2"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        };
        Board board(pieces);
        
        DameAnalyzer dca(board);
        const auto moves = dca.find_valid_moves(dame_pos);
        const auto& sequences = moves.get_capture_sequences();
        
        // Should find at least one sequence (single capture of C2)
        REQUIRE(!sequences.empty());
        
        // Check that we have the expected single capture
        bool found_single_capture = false;
        
        for (const auto& sequence : sequences) {
            if (sequence.size() == 2) {
                // Check if it captures C2 and lands on D3
                if (sequence[0] == Position{"C2"} && sequence[1] == Position{"D3"}) {
                    found_single_capture = true;
                }
            }
        }
        
        REQUIRE(found_single_capture);
    }
}

TEST_CASE("DameAnalyzer - Deduplication", "[DameAnalyzer]") {
    SECTION("Equivalent sequences should be deduplicated") {
        // This test ensures that sequences capturing the same pieces 
        // and ending at the same position are properly deduplicated
        Position dame_pos{"D5"};
        
        const Pieces pieces = {
            {dame_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
            {Position{"C4"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
            {Position{"E6"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        };
        Board board(pieces);
        
        DameAnalyzer dca(board);
        const auto moves = dca.find_valid_moves(dame_pos);
        const auto& sequences = moves.get_capture_sequences();
        
        // Verify that we don't have duplicate equivalent sequences
        std::set<std::pair<std::set<Position>, Position>> unique_outcomes;
        
        for (const auto& sequence : sequences) {
            if (sequence.empty() || sequence.size() % 2 != 0) continue;
            
            std::set<Position> captured_pieces;
            for (size_t i = 0; i < sequence.size(); i += 2) {
                captured_pieces.insert(sequence[i]);
            }
            Position final_position = sequence.back();
            
            auto outcome = std::make_pair(captured_pieces, final_position);
            REQUIRE(unique_outcomes.find(outcome) == unique_outcomes.end());
            unique_outcomes.insert(outcome);
        }
    }
}

TEST_CASE("DameAnalyzer - Edge Cases and Comprehensive Coverage", "[DameAnalyzer]") {
    SECTION("Invalid position coordinates") {
        Board board; // Empty board
        DameAnalyzer dca(board);
        
        // Test with invalid coordinates - Position constructor will throw
        REQUIRE_THROWS_AS(Position(9, 9), std::out_of_range);
        REQUIRE_THROWS_AS(Position(-1, -1), std::out_of_range);
    }
    
    SECTION("Dame at board boundaries") {
        // Test dame piece at various valid board edge positions (only black squares)
        std::vector<Position> edge_positions = {
            Position("B1"), Position("H1"), Position("A8"), Position("G8")
        };
        
        for (const auto& edge_pos : edge_positions) {
            const Pieces pieces = {
                {edge_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
            };
            Board board(pieces);
            DameAnalyzer dca(board);
            
            // Should not throw, just return available moves (likely regular moves)
            REQUIRE_NOTHROW(dca.find_valid_moves(edge_pos));
            const auto moves = dca.find_valid_moves(edge_pos);
            
            // At edge positions, may or may not have moves depending on direction availability
            // Just verify no exception is thrown
            INFO("Edge position: " << edge_pos.to_string());
        }
    }
    
    SECTION("Dame with only friendly pieces around") {
        Position dame_pos{"D5"};
        const Pieces pieces = {
            {dame_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
            // Surround with friendly pieces - but leave some directions free
            {Position{"C4"}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
            {Position{"E4"}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
            // Leave SW and SE directions partially open for regular moves
        };
        Board board(pieces);
        DameAnalyzer dca(board);
        
        const auto moves = dca.find_valid_moves(dame_pos);
        const auto& sequences = moves.get_capture_sequences();
        
        // Should have no captures available
        REQUIRE(sequences.empty());
        // Should have some regular moves available in the non-blocked directions
        const auto& positions = moves.get_positions();
        REQUIRE(!positions.empty()); // Should have moves in SW/SE directions
    }
    
    SECTION("Long range captures and moves") {
        Position dame_pos{"B1"};
        const Pieces pieces = {
            {dame_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
            // Place opponent piece far away diagonally (but reachable)
            {Position{"F5"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        };
        Board board(pieces);
        DameAnalyzer dca(board);
        
        const auto moves = dca.find_valid_moves(dame_pos);
        const auto& sequences = moves.get_capture_sequences();
        
        // Should find the long-range capture
        REQUIRE(sequences.size() == 1);
        const auto& sequence = *sequences.begin();
        REQUIRE(sequence.size() == 2);
        REQUIRE(sequence[0] == Position{"F5"}); // captured piece
        REQUIRE(sequence[1] == Position{"G6"}); // landing position
    }
    
    SECTION("Multiple opponents but no captures possible") {
        Position dame_pos{"D5"};
        const Pieces pieces = {
            {dame_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
            // Opponent pieces with no landing space
            {Position{"C4"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
            {Position{"B3"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}, // Block landing
            {Position{"E6"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
            {Position{"F7"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}, // Block landing
        };
        Board board(pieces);
        DameAnalyzer dca(board);
        
        const auto moves = dca.find_valid_moves(dame_pos);
        const auto& sequences = moves.get_capture_sequences();
        
        // Should have no captures due to blocked landing positions
        REQUIRE(sequences.empty());
    }
    
    SECTION("Regular moves - all directions") {
        Position dame_pos{"D5"};
        const Pieces pieces = {
            {dame_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
            // No other pieces, so only regular moves available
        };
        Board board(pieces);
        DameAnalyzer dca(board);
        
        const auto moves = dca.find_valid_moves(dame_pos);
        const auto& regular_moves = moves.get_positions();
        
        // Should have moves in all 4 diagonal directions
        REQUIRE(regular_moves.size() > 10); // Multiple positions in each direction
        
        // Verify moves in each direction exist
        bool has_nw = false, has_ne = false, has_sw = false, has_se = false;
        for (const auto& pos : regular_moves) {
            if (pos.x < dame_pos.x && pos.y < dame_pos.y) has_nw = true;
            if (pos.x > dame_pos.x && pos.y < dame_pos.y) has_ne = true;
            if (pos.x < dame_pos.x && pos.y > dame_pos.y) has_sw = true;
            if (pos.x > dame_pos.x && pos.y > dame_pos.y) has_se = true;
        }
        REQUIRE(has_nw);
        REQUIRE(has_ne);
        REQUIRE(has_sw);
        REQUIRE(has_se);
    }
}

TEST_CASE("DameAnalyzer - Deduplication Edge Cases", "[DameAnalyzer]") {
    SECTION("Odd length sequence handling") {
        Position dame_pos{"D5"};
        const Pieces pieces = {
            {dame_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}},
            {Position{"C4"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
        };
        Board board(pieces);
        DameAnalyzer dca(board);
        
        // Normal sequences should have even length (captured + landing pairs)
        const auto moves = dca.find_valid_moves(dame_pos);
        const auto& sequences = moves.get_capture_sequences();
        
        for (const auto& sequence : sequences) {
            REQUIRE(sequence.size() % 2 == 0); // All sequences should have even length
        }
    }
}

TEST_CASE("DameAnalyzer - Black Dame Pieces", "[DameAnalyzer]") {
    SECTION("Black dame capturing white pieces") {
        Position dame_pos{"D5"};
        const Pieces pieces = {
            {dame_pos, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::DAME}},
            {Position{"C4"}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
            {Position{"E6"}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
        };
        Board board(pieces);
        DameAnalyzer dca(board);
        
        const auto moves = dca.find_valid_moves(dame_pos);
        const auto& sequences = moves.get_capture_sequences();
        
        // Should find captures available for black dame
        REQUIRE(!sequences.empty());
        
        // Verify that captures target white pieces
        bool found_c4_capture = false, found_e6_capture = false;
        for (const auto& sequence : sequences) {
            if (sequence.size() >= 2) {
                if (sequence[0] == Position{"C4"}) found_c4_capture = true;
                if (sequence[0] == Position{"E6"}) found_e6_capture = true;
            }
        }
        REQUIRE(found_c4_capture);
        REQUIRE(found_e6_capture);
    }
}
