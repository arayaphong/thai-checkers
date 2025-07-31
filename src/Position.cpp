#include "Position.h"
#include <stdexcept>
#include <string>

// Non-constexpr functions that can't be evaluated at compile time
Position Position::from_index(int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= 4 * board_size) {
        throw std::out_of_range("Index out of range for Position");
    }
    const auto y = index / 4;
    const auto x_base = 2 * (index % 4);
    const auto x = x_base + (((x_base + y) % 2) == 0);

    return Position{x, y};
}

Position Position::from_hash(std::size_t hash_value) {
    if (hash_value >= 32) {  // Maximum possible hash value is 31 (32 black squares)
        throw std::out_of_range("Hash value out of range for Position");
    }
    const int y = static_cast<int>(hash_value / (board_size / 2));
    const int x = static_cast<int>((hash_value % (board_size / 2)) * 2);
    
    // Adjust x based on the row to ensure we're on a black square
    const int adjusted_x = x + ((x + y) % 2 == 0 ? 1 : 0);
    
    return Position{adjusted_x, y};
}

std::string Position::to_string() const noexcept {
    return std::string(1, static_cast<char>('A' + x)) + std::to_string(y + 1);
}
