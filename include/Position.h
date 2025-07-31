#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <stdexcept>
#include <cstddef>
#include <compare>

struct Position {
    int x;
    int y;

    static constexpr std::size_t board_size = 8;

    [[nodiscard]] static constexpr bool is_valid(int x, int y) noexcept {
        // Allow only black squares (odd sum of coordinates)
        return x >= 0 && x < static_cast<int>(board_size) && 
            y >= 0 && y < static_cast<int>(board_size) && 
            (x + y) % 2 == 1;
    }

    [[nodiscard]] static constexpr bool is_valid(const char pos_str[2]) noexcept {
        if (!pos_str) {
            return false;
        }
        const char col = pos_str[0];
        const char row = pos_str[1];
        const auto valid_str = (col >= 'A' && col <= 'H' && row >= '1' && row <= '8');
        return valid_str && is_valid(col - 'A',  row - '1');
    }

    [[nodiscard]] static constexpr Position from_coords(int x, int y) {
        if (!is_valid(x, y)) {
            throw std::out_of_range("Invalid coordinates for Position");
        }
        return Position{x, y};
    }

    [[nodiscard]] static Position from_index(int index);
    [[nodiscard]] static Position from_hash(std::size_t hash_value);

    constexpr Position() noexcept = default;
    
    constexpr Position(int x_, int y_) : x(x_), y(y_) {
        if (!is_valid(x, y)) {
            throw std::out_of_range("Invalid position");
        }
    }
    
    constexpr Position(const char pos_str[2]) : x(pos_str[0] - 'A'), y(pos_str[1] - '1') {
        if (!pos_str || !is_valid(x, y)) {
            throw std::out_of_range("Invalid position string");
        }
    }

    [[nodiscard]] constexpr std::size_t hash() const noexcept {
        return (x / 2) + ((Position::board_size / 2) * y);
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return is_valid(x, y);
    }
    
    [[nodiscard]] std::string to_string() const noexcept;
    
    [[nodiscard]] constexpr std::strong_ordering operator<=>(const Position& other) const noexcept {
        return hash() <=> other.hash();
    }
    [[nodiscard]] bool operator==(const Position& other) const noexcept = default;
};

namespace std {
    template <>
    struct hash<Position> {
        std::size_t operator()(const Position& p) const {
            return p.hash();
        }
    };
}