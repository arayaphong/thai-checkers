#include "Position.h"
#include <stdexcept>
#include <string>
#include <format>
#include <cstdint>

// C++20 improvements to non-constexpr functions
Position Position::from_index(int index) noexcept {
    if (index < 0) index = 0;
    if (static_cast<std::size_t>(index) >= max_positions()) index = static_cast<int>(max_positions() - 1);
    return Position{static_cast<std::uint8_t>(index)};
}

std::string Position::to_string() const noexcept {
    return std::format("{}{}", static_cast<char>('A' + x()), y() + 1);
}
