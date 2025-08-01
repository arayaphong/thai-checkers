#include "Position.h"
#include <stdexcept>
#include <string>
#include <format>

// C++20 improvements to non-constexpr functions
Position Position::from_index(int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= max_positions()) {
        throw std::out_of_range(std::format("Index {} out of range for Position (max: {})", 
                                          index, max_positions() - 1));
    }
    
    // Modern C++20 structured bindings and constexpr improvements
    const auto y = index / 4;
    const auto x_base = 2 * (index % 4);
    const auto x = x_base + (((x_base + y) % 2) == 0 ? 1 : 0);

    return Position{x, y};
}

Position Position::from_hash(std::size_t hash_value) {
    if (hash_value >= max_positions()) {
        throw std::out_of_range(std::format("Hash value {} out of range for Position (max: {})", 
                                          hash_value, max_positions() - 1));
    }
    
    // C++20 improvements with better type safety
    const auto y = static_cast<int>(hash_value / (board_size / 2));
    const auto x = static_cast<int>((hash_value % (board_size / 2)) * 2);
    
    // Adjust x based on the row to ensure we're on a black square
    const auto adjusted_x = x + ((x + y) % 2 == 0 ? 1 : 0);
    
    return Position{adjusted_x, y};
}

std::string Position::to_string() const noexcept {
    // C++20 std::format for more modern string formatting
    try {
        return std::format("{}{}", static_cast<char>('A' + x()), y() + 1);
    } catch (...) {
        // Fallback to traditional method if format fails
        return std::string(1, static_cast<char>('A' + x())) + std::to_string(y() + 1);
    }
}
