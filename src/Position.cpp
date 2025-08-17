#include "Position.h"
#include <cstddef>
#include <string>
#include <format>
#include <cstdint>

// C++20 improvements to non-constexpr functions
auto Position::from_index(int index) noexcept -> Position {
    if (index < 0) { index = 0;
}
    if (static_cast<std::size_t>(index) >= max_positions()) { index = static_cast<int>(max_positions() - 1);
}
    return Position{static_cast<std::uint8_t>(index)};
}

auto Position::to_string() const noexcept -> std::string {
  try {
    return std::format("{}{}", static_cast<char>('A' + x()), y() + 1);
  } catch (const std::format_error& e) {
    // This should not happen with a fixed format string.
    // Return a fallback representation or handle error appropriately.
    return "Error";
  }
}
