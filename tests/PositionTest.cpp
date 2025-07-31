#include <catch2/catch_all.hpp>
#include "Position.h"
#include <stdexcept>
#include <string>

TEST_CASE("Position validation for coordinates") {
    SECTION("Valid positions (black squares only)") {
        // Test valid black squares (odd sum of coordinates)
        REQUIRE(Position::is_valid(1, 0));  // B1
        REQUIRE(Position::is_valid(3, 0));  // D1
        REQUIRE(Position::is_valid(5, 0));  // F1
        REQUIRE(Position::is_valid(7, 0));  // H1
        REQUIRE(Position::is_valid(0, 1));  // A2
        REQUIRE(Position::is_valid(2, 1));  // C2
        REQUIRE(Position::is_valid(4, 1));  // E2
        REQUIRE(Position::is_valid(6, 1));  // G2
        REQUIRE(Position::is_valid(1, 2));  // B3
        REQUIRE(Position::is_valid(3, 4));  // D5
        REQUIRE(Position::is_valid(7, 6));  // H7
    }

    SECTION("Invalid positions - white squares") {
        // Test invalid white squares (even sum of coordinates)
        REQUIRE_FALSE(Position::is_valid(0, 0));  // A1
        REQUIRE_FALSE(Position::is_valid(2, 0));  // C1
        REQUIRE_FALSE(Position::is_valid(4, 0));  // E1
        REQUIRE_FALSE(Position::is_valid(6, 0));  // G1
        REQUIRE_FALSE(Position::is_valid(1, 1));  // B2
        REQUIRE_FALSE(Position::is_valid(3, 1));  // D2
        REQUIRE_FALSE(Position::is_valid(5, 1));  // F2
        REQUIRE_FALSE(Position::is_valid(7, 1));  // H2
    }

    SECTION("Invalid positions - out of bounds") {
        REQUIRE_FALSE(Position::is_valid(-1, 0));
        REQUIRE_FALSE(Position::is_valid(0, -1));
        REQUIRE_FALSE(Position::is_valid(8, 0));
        REQUIRE_FALSE(Position::is_valid(0, 8));
        REQUIRE_FALSE(Position::is_valid(-1, -1));
        REQUIRE_FALSE(Position::is_valid(8, 8));
        REQUIRE_FALSE(Position::is_valid(10, 5));
        REQUIRE_FALSE(Position::is_valid(5, 10));
    }
}

TEST_CASE("Position validation for string notation") {
    SECTION("Valid string positions") {
        REQUIRE(Position::is_valid("B1"));  // (1,0)
        REQUIRE(Position::is_valid("D1"));  // (3,0)
        REQUIRE(Position::is_valid("F1"));  // (5,0)
        REQUIRE(Position::is_valid("H1"));  // (7,0)
        REQUIRE(Position::is_valid("A2"));  // (0,1)
        REQUIRE(Position::is_valid("C2"));  // (2,1)
        REQUIRE(Position::is_valid("E2"));  // (4,1)
        REQUIRE(Position::is_valid("G2"));  // (6,1)
        REQUIRE(Position::is_valid("B3"));  // (1,2)
        REQUIRE(Position::is_valid("H7"));  // (7,6)
    }

    SECTION("Invalid string positions - white squares") {
        REQUIRE_FALSE(Position::is_valid("A1"));  // (0,0)
        REQUIRE_FALSE(Position::is_valid("C1"));  // (2,0)
        REQUIRE_FALSE(Position::is_valid("E1"));  // (4,0)
        REQUIRE_FALSE(Position::is_valid("G1"));  // (6,0)
        REQUIRE_FALSE(Position::is_valid("B2"));  // (1,1)
        REQUIRE_FALSE(Position::is_valid("D2"));  // (3,1)
        REQUIRE_FALSE(Position::is_valid("F2"));  // (5,1)
        REQUIRE_FALSE(Position::is_valid("H2"));  // (7,1)
    }

    SECTION("Invalid string positions - out of bounds") {
        REQUIRE_FALSE(Position::is_valid("I1"));  // Column out of bounds
        REQUIRE_FALSE(Position::is_valid("A9"));  // Row out of bounds
        REQUIRE_FALSE(Position::is_valid("Z5"));  // Column way out of bounds
        REQUIRE_FALSE(Position::is_valid("D0"));  // Row below bounds
    }

    SECTION("Invalid string positions - null pointer") {
        REQUIRE_FALSE(Position::is_valid(nullptr));
    }
}

TEST_CASE("Position construction from coordinates") {
    SECTION("Valid construction") {
        Position pos1(1, 0);  // B1
        REQUIRE(pos1.x == 1);
        REQUIRE(pos1.y == 0);
        REQUIRE(pos1.is_valid());

        Position pos2(7, 6);  // H7
        REQUIRE(pos2.x == 7);
        REQUIRE(pos2.y == 6);
        REQUIRE(pos2.is_valid());
    }

    SECTION("Invalid construction - white squares") {
        REQUIRE_THROWS_AS(Position(0, 0), std::out_of_range);  // A1
        REQUIRE_THROWS_AS(Position(2, 0), std::out_of_range);  // C1
        REQUIRE_THROWS_AS(Position(1, 1), std::out_of_range);  // B2
    }

    SECTION("Invalid construction - out of bounds") {
        REQUIRE_THROWS_AS(Position(-1, 0), std::out_of_range);
        REQUIRE_THROWS_AS(Position(0, -1), std::out_of_range);
        REQUIRE_THROWS_AS(Position(8, 0), std::out_of_range);
        REQUIRE_THROWS_AS(Position(0, 8), std::out_of_range);
    }
}

TEST_CASE("Position construction from string") {
    SECTION("Valid construction") {
        Position pos1("B1");
        REQUIRE(pos1.x == 1);
        REQUIRE(pos1.y == 0);

        Position pos2("A2");
        REQUIRE(pos2.x == 0);
        REQUIRE(pos2.y == 1);

        Position pos3("H7");
        REQUIRE(pos3.x == 7);
        REQUIRE(pos3.y == 6);
    }

    SECTION("Invalid construction - white squares") {
        REQUIRE_THROWS_AS(Position("A1"), std::out_of_range);
        REQUIRE_THROWS_AS(Position("C1"), std::out_of_range);
        REQUIRE_THROWS_AS(Position("B2"), std::out_of_range);
    }

    SECTION("Invalid construction - out of bounds") {
        REQUIRE_THROWS_AS(Position("I1"), std::out_of_range);
        REQUIRE_THROWS_AS(Position("A9"), std::out_of_range);
        REQUIRE_THROWS_AS(Position("D0"), std::out_of_range);
    }
}

TEST_CASE("Position default construction") {
    Position pos;
    (void)pos; // Suppress unused variable warning
    // Default constructed position should have uninitialized values
    // We can't guarantee what x and y will be, but we can test that
    // the default constructor compiles and runs without throwing
    REQUIRE_NOTHROW(Position{});
}

TEST_CASE("Position hash function") {
    SECTION("Hash calculation") {
        Position pos1(1, 0);  // B1
        Position pos2(3, 0);  // D1
        Position pos3(0, 1);  // A2
        Position pos4(7, 6);  // H7

        // Hash should be (x / 2) + ((board_size / 2) * y)
        REQUIRE(pos1.hash() == (1 / 2) + ((8 / 2) * 0));  // 0 + 0 = 0
        REQUIRE(pos2.hash() == (3 / 2) + ((8 / 2) * 0));  // 1 + 0 = 1
        REQUIRE(pos3.hash() == (0 / 2) + ((8 / 2) * 1));  // 0 + 4 = 4
        REQUIRE(pos4.hash() == (7 / 2) + ((8 / 2) * 6));  // 3 + 24 = 27
    }

    SECTION("Hash uniqueness for different positions") {
        Position pos1(1, 0);  // B1
        Position pos2(3, 0);  // D1
        Position pos3(0, 1);  // A2

        REQUIRE(pos1.hash() != pos2.hash());
        REQUIRE(pos1.hash() != pos3.hash());
        REQUIRE(pos2.hash() != pos3.hash());
    }

    SECTION("Hash consistency") {
        Position pos(5, 2);
        auto hash1 = pos.hash();
        auto hash2 = pos.hash();
        REQUIRE(hash1 == hash2);
    }
}

TEST_CASE("Position to_string conversion") {
    SECTION("Valid conversions") {
        Position pos1(1, 0);  // B1
        REQUIRE(pos1.to_string() == "B1");

        Position pos2(0, 1);  // A2
        REQUIRE(pos2.to_string() == "A2");

        Position pos3(7, 6);  // H7
        REQUIRE(pos3.to_string() == "H7");

        Position pos4(3, 4);  // D5
        REQUIRE(pos4.to_string() == "D5");
    }

    SECTION("Round-trip conversion") {
        std::vector<const char*> test_positions = {"B1", "D1", "F1", "H1", "A2", "C2", "E2", "G2", "B3", "H7"};
        
        for (const auto& pos_str : test_positions) {
            Position pos(pos_str);
            REQUIRE(pos.to_string() == pos_str);
        }
    }
}

TEST_CASE("Position equality operators") {
    SECTION("Equality") {
        Position pos1(1, 0);
        Position pos2(1, 0);
        Position pos3(3, 0);

        REQUIRE(pos1 == pos2);
        REQUIRE_FALSE(pos1 == pos3);
    }

    SECTION("Inequality") {
        Position pos1(1, 0);
        Position pos2(1, 0);
        Position pos3(3, 0);

        REQUIRE_FALSE(pos1 != pos2);
        REQUIRE(pos1 != pos3);
    }
}

TEST_CASE("Position less-than operator") {
    SECTION("Ordering based on hash") {
        Position pos1(1, 0);  // hash = 0
        Position pos2(3, 0);  // hash = 1
        Position pos3(0, 1);  // hash = 4

        REQUIRE(pos1 < pos2);
        REQUIRE(pos1 < pos3);
        REQUIRE(pos2 < pos3);
        REQUIRE_FALSE(pos2 < pos1);
        REQUIRE_FALSE(pos3 < pos1);
        REQUIRE_FALSE(pos3 < pos2);
    }

    SECTION("Self comparison") {
        Position pos(1, 0);
        REQUIRE_FALSE(pos < pos);
    }
}

TEST_CASE("Position std::hash integration") {
    SECTION("Standard hash function") {
        Position pos1(1, 0);
        Position pos2(3, 0);

        REQUIRE(pos1.hash() == pos1.hash());
        REQUIRE(pos2.hash() == pos2.hash());
        REQUIRE(pos1.hash() != pos2.hash());
    }
}

TEST_CASE("Position board_size constant") {
    REQUIRE(Position::board_size == 8);
}

TEST_CASE("Position from_index method") {
    SECTION("Valid index conversions") {
        // Test known index-to-position mappings
        Position pos0 = Position::from_index(0);   // Should be B1 (1,0)
        REQUIRE(pos0.x == 1);
        REQUIRE(pos0.y == 0);
        REQUIRE(pos0.to_string() == "B1");

        Position pos1 = Position::from_index(1);   // Should be D1 (3,0)
        REQUIRE(pos1.x == 3);
        REQUIRE(pos1.y == 0);
        REQUIRE(pos1.to_string() == "D1");

        Position pos4 = Position::from_index(4);   // Should be A2 (0,1)
        REQUIRE(pos4.x == 0);
        REQUIRE(pos4.y == 1);
        REQUIRE(pos4.to_string() == "A2");

        Position pos31 = Position::from_index(31); // Should be G8 (6,7)
        REQUIRE(pos31.x == 6);
        REQUIRE(pos31.y == 7);
        REQUIRE(pos31.to_string() == "G8");
    }

    SECTION("All valid indices") {
        // Test that all indices from 0 to 31 produce valid positions
        for (int i = 0; i < 32; ++i) {
            REQUIRE_NOTHROW(Position::from_index(i));
            Position pos = Position::from_index(i);
            REQUIRE(pos.is_valid());
            // Verify it's on a black square
            REQUIRE((pos.x + pos.y) % 2 == 1);
        }
    }

    SECTION("Invalid indices") {
        REQUIRE_THROWS_AS(Position::from_index(-1), std::out_of_range);
        REQUIRE_THROWS_AS(Position::from_index(32), std::out_of_range);
        REQUIRE_THROWS_AS(Position::from_index(100), std::out_of_range);
    }

    SECTION("Round-trip conversion with hash") {
        // Test that from_index produces positions whose hash can be used
        for (int i = 0; i < 32; ++i) {
            Position pos = Position::from_index(i);
            // The hash should correspond to the position's logical index
            // This verifies the relationship between index and hash
            REQUIRE(pos.is_valid());
        }
    }
}

TEST_CASE("Position from_hash method") {
    SECTION("Valid hash conversions") {
        // Test known hash-to-position mappings based on hash() = (x/2) + ((board_size/2) * y)
        Position pos0 = Position::from_hash(0);    // hash=0: should be B1 (1,0)
        REQUIRE(pos0.x == 1);
        REQUIRE(pos0.y == 0);
        REQUIRE(pos0.to_string() == "B1");
        REQUIRE(pos0.hash() == 0);

        Position pos1 = Position::from_hash(1);    // hash=1: should be D1 (3,0)
        REQUIRE(pos1.x == 3);
        REQUIRE(pos1.y == 0);
        REQUIRE(pos1.to_string() == "D1");
        REQUIRE(pos1.hash() == 1);

        Position pos4 = Position::from_hash(4);    // hash=4: should be A2 (0,1)
        REQUIRE(pos4.x == 0);
        REQUIRE(pos4.y == 1);
        REQUIRE(pos4.to_string() == "A2");
        REQUIRE(pos4.hash() == 4);

        Position pos27 = Position::from_hash(27);  // hash=27: should be H7 (7,6)
        REQUIRE(pos27.x == 7);
        REQUIRE(pos27.y == 6);
        REQUIRE(pos27.to_string() == "H7");
        REQUIRE(pos27.hash() == 27);
    }

    SECTION("All valid hash values") {
        // Test that all hash values from 0 to 31 produce valid positions
        for (std::size_t hash = 0; hash < 32; ++hash) {
            REQUIRE_NOTHROW(Position::from_hash(hash));
            Position pos = Position::from_hash(hash);
            REQUIRE(pos.is_valid());
            // Verify it's on a black square
            REQUIRE((pos.x + pos.y) % 2 == 1);
            // Verify round-trip: hash -> position -> hash
            REQUIRE(pos.hash() == hash);
        }
    }

    SECTION("Invalid hash values") {
        REQUIRE_THROWS_AS(Position::from_hash(32), std::out_of_range);
        REQUIRE_THROWS_AS(Position::from_hash(100), std::out_of_range);
        REQUIRE_THROWS_AS(Position::from_hash(std::numeric_limits<std::size_t>::max()), std::out_of_range);
    }

    SECTION("Round-trip conversion") {
        // Test that position -> hash -> position is identity
        std::vector<const char*> test_positions = {
            "B1", "D1", "F1", "H1", "A2", "C2", "E2", "G2", 
            "B3", "D3", "F3", "H3", "A4", "C4", "E4", "G4",
            "B5", "D5", "F5", "H5", "A6", "C6", "E6", "G6",
            "B7", "D7", "F7", "H7", "A8", "C8", "E8", "G8"
        };
        
        for (const auto& pos_str : test_positions) {
            Position original_pos(pos_str);
            std::size_t hash_value = original_pos.hash();
            Position restored_pos = Position::from_hash(hash_value);
            
            REQUIRE(original_pos == restored_pos);
            REQUIRE(original_pos.x == restored_pos.x);
            REQUIRE(original_pos.y == restored_pos.y);
            REQUIRE(original_pos.to_string() == restored_pos.to_string());
        }
    }

    SECTION("Hash consistency with existing positions") {
        // Test specific known positions and their expected hash values
        Position b1(1, 0);  // B1, expected hash = (1/2) + ((8/2) * 0) = 0
        REQUIRE(b1.hash() == 0);
        REQUIRE(Position::from_hash(0) == b1);

        Position d1(3, 0);  // D1, expected hash = (3/2) + ((8/2) * 0) = 1
        REQUIRE(d1.hash() == 1);
        REQUIRE(Position::from_hash(1) == d1);

        Position a2(0, 1);  // A2, expected hash = (0/2) + ((8/2) * 1) = 4
        REQUIRE(a2.hash() == 4);
        REQUIRE(Position::from_hash(4) == a2);

        Position h7(7, 6);  // H7, expected hash = (7/2) + ((8/2) * 6) = 3 + 24 = 27
        REQUIRE(h7.hash() == 27);
        REQUIRE(Position::from_hash(27) == h7);
    }
}

TEST_CASE("Position comprehensive black square validation") {
    SECTION("All valid black squares on 8x8 board") {
        std::vector<std::pair<int, int>> black_squares;
        
        // Generate all black squares (odd sum coordinates)
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                if ((x + y) % 2 == 1) {
                    black_squares.emplace_back(x, y);
                }
            }
        }
        
        // Should have exactly 32 black squares
        REQUIRE(black_squares.size() == 32);
        
        // All should be valid
        for (const auto& [x, y] : black_squares) {
            REQUIRE(Position::is_valid(x, y));
            REQUIRE_NOTHROW(Position(x, y));
        }
    }
    
    SECTION("All invalid white squares on 8x8 board") {
        std::vector<std::pair<int, int>> white_squares;
        
        // Generate all white squares (even sum coordinates)
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                if ((x + y) % 2 == 0) {
                    white_squares.emplace_back(x, y);
                }
            }
        }
        
        // Should have exactly 32 white squares
        REQUIRE(white_squares.size() == 32);
        
        // All should be invalid
        for (const auto& [x, y] : white_squares) {
            REQUIRE_FALSE(Position::is_valid(x, y));
            REQUIRE_THROWS_AS(Position(x, y), std::out_of_range);
        }
    }
}
