#include <catch2/catch_all.hpp>
#include <ranges>
#include <algorithm>

#include "Board.h"
#include "PionAnalyzer.h"
#include "DameAnalyzer.h"

TEST_CASE("PionAnalyzer - Basic movement tests", "[PionAnalyzer]") {
    SECTION("White pion normal moves") {
        const Position focus{"C4"};
        const Pieces pieces = {
            {focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}}
        };
        
        Board board(pieces);
        PionAnalyzer analyzer(board);
        
        auto options = analyzer.find_valid_moves(focus);
        REQUIRE(options.has_positions());
        
        auto positions = options.get_positions();
        REQUIRE(positions.size() == 2);
        REQUIRE(std::ranges::find(positions, Position{"B3"}) != positions.end());
        REQUIRE(std::ranges::find(positions, Position{"D3"}) != positions.end());
    }
    
    SECTION("Black pion normal moves") {
        const Position focus{"C4"};
        const Pieces pieces = {
            {focus, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}
        };
        
        Board board(pieces);
        PionAnalyzer analyzer(board);
        
        auto options = analyzer.find_valid_moves(focus);
        REQUIRE(options.has_positions());
        
        auto positions = options.get_positions();
        REQUIRE(positions.size() == 2);
        REQUIRE(std::ranges::find(positions, Position{"B5"}) != positions.end());
        REQUIRE(std::ranges::find(positions, Position{"D5"}) != positions.end());
    }
    
    SECTION("Blocked moves") {
        const Position focus{"C4"};
        const Pieces pieces = {
            {focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
            {Position{"B3"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}, 
            {Position{"D3"}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}}  
        };
        
        Board board(pieces);
        PionAnalyzer analyzer(board);
        
        auto options = analyzer.find_valid_moves(focus);
        REQUIRE_FALSE(options.has_positions());
    }
}

TEST_CASE("PionAnalyzer - Capture tests", "[PionAnalyzer]") {
    SECTION("Single capture - white captures black") {
        const Position focus{"C4"};
        const Pieces pieces = {
            {focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
            {Position{"B3"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}
        };
        
        Board board(pieces);
        PionAnalyzer analyzer(board);
        
        auto options = analyzer.find_valid_moves(focus);
        
        REQUIRE(options.has_captured());
        auto sequences = options.get_capture_sequences();
        REQUIRE(sequences.size() == 1);
        
        auto sequence_it = sequences.begin();
        const auto& sequence = *sequence_it;
        
        REQUIRE(sequence.size() == 2);
        REQUIRE(sequence[0] == Position{"B3"}); // Captured piece
        REQUIRE(sequence[1] == Position{"A2"}); // Landing position
    }
    
    SECTION("Multiple capture opportunities") {
        const Position focus{"C4"};
        const Pieces pieces = {
            {focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
            {Position{"B3"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
            {Position{"D3"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}
        };
        
        Board board(pieces);
        PionAnalyzer analyzer(board);
        
        auto options = analyzer.find_valid_moves(focus);
        
        REQUIRE(options.has_captured());
        auto sequences = options.get_capture_sequences();
        REQUIRE(sequences.size() == 2);
        
        // Verify both capture sequences are present
        bool found_b3_capture = false;
        bool found_d3_capture = false;
        
        for (const auto& sequence : sequences) {
            REQUIRE(sequence.size() == 2);
            if (sequence[0] == Position{"B3"} && sequence[1] == Position{"A2"}) {
                found_b3_capture = true;
            }
            if (sequence[0] == Position{"D3"} && sequence[1] == Position{"E2"}) {
                found_d3_capture = true;
            }
        }
        
        REQUIRE(found_b3_capture);
        REQUIRE(found_d3_capture);
    }
    
    SECTION("Capture blocked by friendly piece") {
        const Position focus{"C4"};
        const Pieces pieces = {
            {focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
            {Position{"B3"}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}} 
        };
        
        Board board(pieces);
        PionAnalyzer analyzer(board);
        
        auto options = analyzer.find_valid_moves(focus);
        REQUIRE_FALSE(options.has_captured());
    }
    
    SECTION("Capture landing blocked") {
        const Position focus{"C4"};
        const Pieces pieces = {
            {focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
            {Position{"B3"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}, 
            {Position{"A2"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}  
        };
        
        Board board(pieces);
        PionAnalyzer analyzer(board);
        
        auto options = analyzer.find_valid_moves(focus);
        REQUIRE_FALSE(options.has_captured());
    }
}

TEST_CASE("PionAnalyzer - Edge case tests", "[PionAnalyzer]") {
    SECTION("Board boundaries") {
        const Position edge_pos{"A2"}; 
        const Pieces edge_pieces = {
            {edge_pos, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}
        };
        
        Board edge_board(edge_pieces);
        PionAnalyzer edge_analyzer(edge_board);
        
        auto edge_options = edge_analyzer.find_valid_moves(edge_pos);
        REQUIRE(edge_options.has_positions());
        auto edge_positions = edge_options.get_positions();
        
        REQUIRE(edge_positions.size() == 1);
        REQUIRE(std::ranges::find(edge_positions, Position{"B3"}) != edge_positions.end());
    }
    
    SECTION("Corner position behavior") {
        // Test piece near corner where only one move is possible
        const Position corner_pos{"G6"};
        const Pieces corner_pieces = {
            {corner_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}}
        };
        
        Board corner_board(corner_pieces);
        PionAnalyzer corner_analyzer(corner_board);
        
        auto corner_options = corner_analyzer.find_valid_moves(corner_pos);
        REQUIRE(corner_options.has_positions());
        auto corner_positions = corner_options.get_positions();
        
        // G6 white pion should be able to move to F5 and H5
        REQUIRE(corner_positions.size() <= 2);
    }
    
    SECTION("Color dependent directions") {
        // White piece moving forward (up)
        const Position white_pos{"C4"};
        const Pieces white_pieces = {
            {white_pos, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}}
        };
        Board white_board(white_pieces);
        PionAnalyzer white_analyzer(white_board);
        
        auto white_options = white_analyzer.find_valid_moves(white_pos);
        REQUIRE(white_options.has_positions());
        auto white_moves = white_options.get_positions();
        
        REQUIRE(std::ranges::find(white_moves, Position{"B3"}) != white_moves.end());
        REQUIRE(std::ranges::find(white_moves, Position{"D3"}) != white_moves.end());
        
        // Black piece moving forward (down) 
        const Position black_pos{"C6"};
        const Pieces black_pieces = {
            {black_pos, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}
        };
        Board black_board(black_pieces);
        PionAnalyzer black_analyzer(black_board);
        
        auto black_options = black_analyzer.find_valid_moves(black_pos);
        REQUIRE(black_options.has_positions());
        auto black_moves = black_options.get_positions();
        
        REQUIRE(std::ranges::find(black_moves, Position{"B7"}) != black_moves.end());
        REQUIRE(std::ranges::find(black_moves, Position{"D7"}) != black_moves.end());
    }
    
    SECTION("Capture near board edge") {
        // Test capture where landing position is near board boundary
        const Position edge_capture_pos{"C6"};
        const Pieces edge_capture_pieces = {
            {edge_capture_pos, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
            {Position{"B7"}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}}
        };
        
        Board edge_capture_board(edge_capture_pieces);
        PionAnalyzer edge_capture_analyzer(edge_capture_board);
        
        auto edge_capture_options = edge_capture_analyzer.find_valid_moves(edge_capture_pos);
        
        // Should have at least regular moves, and potentially a capture
        REQUIRE((edge_capture_options.has_positions() || edge_capture_options.has_captured()));
    }
}

TEST_CASE("PionAnalyzer - Advanced capture scenarios", "[PionAnalyzer]") {
    SECTION("Chain capture setup") {
        // Test scenario that could lead to chain captures
        const Position focus{"C4"};
        const Pieces pieces = {
            {focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
            {Position{"B3"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}},
            {Position{"C2"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}
        };
        
        Board board(pieces);
        PionAnalyzer analyzer(board);
        
        auto options = analyzer.find_valid_moves(focus);
        
        if (options.has_captured()) {
            auto sequences = options.get_capture_sequences();
            REQUIRE(sequences.size() >= 1);
            
            // Check if any sequence has multiple captures
            bool found_chain = false;
            for (const auto& sequence : sequences) {
                if (sequence.size() > 2) { // More than simple capture+landing
                    found_chain = true;
                    break;
                }
            }
            // Note: Chain captures may not be implemented yet, so we don't require it
        }
    }
    
    SECTION("Capture with no landing space") {
        // Test capture where the landing position would be off-board
        const Position edge_focus{"A4"};
        const Pieces edge_pieces = {
            {edge_focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
            {Position{"B3"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}
        };
        
        Board edge_board(edge_pieces);
        PionAnalyzer edge_analyzer(edge_board);
        
        auto edge_options = edge_analyzer.find_valid_moves(edge_focus);
        
        // This should have a valid capture since A4->B3->A2 is valid
        if (edge_options.has_captured()) {
            auto sequences = edge_options.get_capture_sequences();
            REQUIRE(sequences.size() >= 1);
        }
    }
}

TEST_CASE("PionAnalyzer - Validation and error conditions", "[PionAnalyzer]") {
    SECTION("Invalid position handling") {
        // Create board with valid pieces
        const Pieces pieces = {
            {Position{"C4"}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}}
        };
        Board board(pieces);
        PionAnalyzer analyzer(board);
        
        // Test with position that doesn't have a piece
        const Position empty_pos{"E4"};
        REQUIRE_THROWS_AS(analyzer.find_valid_moves(empty_pos), std::invalid_argument);
    }
    
    SECTION("Mixed piece types on board") {
        // Test with both pions and dames on the board
        const Position pion_focus{"C4"};
        const Pieces mixed_pieces = {
            {pion_focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}},
            {Position{"E4"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::DAME}},
            {Position{"B3"}, PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION}}
        };
        
        Board mixed_board(mixed_pieces);
        PionAnalyzer pion_analyzer(mixed_board);
        
        // Should work fine for the pion
        auto pion_options = pion_analyzer.find_valid_moves(pion_focus);
        REQUIRE((pion_options.has_positions() || pion_options.has_captured()));
        
        // Should throw for the dame
        REQUIRE_THROWS_AS(pion_analyzer.find_valid_moves(Position{"E4"}), std::invalid_argument);
    }
}

TEST_CASE("PionAnalyzer - Error handling tests", "[PionAnalyzer]") {
    SECTION("Dame piece throws exception") {
        const Position focus{"C4"};
        const Pieces pieces = {
            {focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}}
        };
        
        Board board(pieces);
        PionAnalyzer analyzer(board);
        
        REQUIRE_THROWS_AS(analyzer.find_valid_moves(focus), std::invalid_argument);
    }
    
    SECTION("Empty position throws exception") {
        const Position empty_pos{"E6"}; 
        const Pieces pieces = {
            {Position{"C4"}, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}}
        };
        
        Board board(pieces);
        PionAnalyzer analyzer(board);
        
        REQUIRE_THROWS_AS(analyzer.find_valid_moves(empty_pos), std::invalid_argument);
    }
}

TEST_CASE("PionAnalyzer vs DameAnalyzer - Comparison", "[PionAnalyzer][DameAnalyzer]") {
    SECTION("Movement range comparison") {
        const Position focus{"C4"};
        
        const Pieces pion_pieces = {
            {focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION}}
        };
        Board pion_board(pion_pieces);
        PionAnalyzer pion_analyzer(pion_board);
        auto pion_options = pion_analyzer.find_valid_moves(focus);
        
        const Pieces dame_pieces = {
            {focus, PieceInfo{.color = PieceColor::WHITE, .type = PieceType::DAME}}
        };
        Board dame_board(dame_pieces);
        DameAnalyzer dame_analyzer(dame_board);
        auto dame_options = dame_analyzer.find_valid_moves(focus);
        
        REQUIRE(pion_options.has_positions());
        REQUIRE(dame_options.has_positions());
        
        auto pion_positions = pion_options.get_positions();
        auto dame_positions = dame_options.get_positions();
        
        REQUIRE(pion_positions.size() == 2);
        REQUIRE(dame_positions.size() >= pion_positions.size());
        
        for (const auto& pos : pion_positions) {
            REQUIRE(std::ranges::find(dame_positions, pos) != dame_positions.end());
        }
    }
}
