#include "Position.h"
#include <stdexcept>
#include <string>
#include <format>
#include <cstdint>

// C++20 improvements to non-constexpr functions
Position Position::from_index(int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= max_positions()) {
        throw std::out_of_range(std::format("Index {} out of range for Position (max: {})", 
                                          index, max_positions() - 1));
    }
    
    return Position{static_cast<std::uint8_t>(index)};
}

Position Position::from_hash(std::size_t hash_value) {
    if (hash_value >= max_positions()) {
        throw std::out_of_range(std::format("Hash value {} out of range for Position (max: {})", 
                                          hash_value, max_positions() - 1));
    }
    
    return Position{static_cast<std::uint8_t>(hash_value)};
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
