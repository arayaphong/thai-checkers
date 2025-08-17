#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <compare>
#include <format>
#include <concepts>
#include <array>
#include <ranges>

// C++20 concepts for type safety
template <typename T>
concept Coordinate = std::integral<T> && requires(T t) {
    { t >= 0 } -> std::convertible_to<bool>;
    { static_cast<int>(t) } -> std::same_as<int>;
};

template <typename T>
concept PositionStringLike = requires(T t) {
    { t[0] } -> std::convertible_to<char>;
    { t[1] } -> std::convertible_to<char>;
};

class Position {
  private:
    std::uint8_t index_;

    // Helper methods to convert between index and coordinates
    [[nodiscard]] constexpr int index_to_x(std::uint8_t idx) const noexcept {
        const auto y = idx / (board_size / 2);
        const auto x_base = (idx % (board_size / 2)) * 2;
        return x_base + ((x_base + y) % 2 == 0 ? 1 : 0);
    }

    [[nodiscard]] constexpr int index_to_y(std::uint8_t idx) const noexcept { return idx / (board_size / 2); }

    [[nodiscard]] static constexpr std::uint8_t coords_to_index(int x, int y) noexcept {
        return static_cast<std::uint8_t>((x / 2) + ((board_size / 2) * y));
    }

  public:
    // Modern C++20 constexpr for compile-time constants
    static constexpr std::size_t board_size = 8;

    // C++20 constexpr improvements
    [[nodiscard]] static constexpr std::size_t max_positions() noexcept {
        return (board_size * board_size) / 2; // Only black squares
    }

    // C++20 concepts in action
    template <Coordinate CoordType> [[nodiscard]] static constexpr bool is_valid(CoordType x, CoordType y) noexcept {
        // Allow only black squares (odd sum of coordinates)
        return x >= 0 && x < static_cast<CoordType>(board_size) && y >= 0 && y < static_cast<CoordType>(board_size) &&
               (x + y) % 2 == 1;
    }

    // C++20 concepts for string-like types
    template <PositionStringLike StringType>
    [[nodiscard]] static constexpr bool is_valid(const StringType& pos_str) noexcept
        requires requires {
            pos_str[0];
            pos_str[1];
        }
    {
        const char col = pos_str[0];
        const char row = pos_str[1];
        const auto valid_str = (col >= 'A' && col <= 'H' && row >= '1' && row <= '8');
        return valid_str && is_valid(col - 'A', row - '1');
    }

    // Legacy C-array support without requires clause
    [[nodiscard]] static constexpr bool is_valid(const char pos_str[2]) noexcept {
        if (!pos_str) { return false; }
        const char col = pos_str[0];
        const char row = pos_str[1];
        const auto valid_str = (col >= 'A' && col <= 'H' && row >= '1' && row <= '8');
        return valid_str && is_valid(col - 'A', row - '1');
    }

    // Factory functions with concepts
    template <Coordinate CoordType> [[nodiscard]] static constexpr Position from_coords(CoordType x, CoordType y) {
        return Position{static_cast<int>(x), static_cast<int>(y)};
    }

    [[nodiscard]] static Position from_index(int index) noexcept;

    // C++20 designated initializers support through factory
    struct CoordinatePair {
        int x;
        int y;
    };

    [[nodiscard]] static constexpr Position from_pair(const CoordinatePair& coords) {
        return Position{coords.x, coords.y};
    }

    // Default constructor creates first valid position (index 0)
    constexpr Position() noexcept : index_{0} {}

    // Direct index constructor
    explicit constexpr Position(std::uint8_t index) noexcept : index_{index} {}

    // C++20 requires clause for constructor validation
    template<Coordinate CoordType>
    constexpr Position(CoordType x, CoordType y)
        requires(std::convertible_to<CoordType, int>)
        : index_{[&]() -> std::uint8_t {
            const int xi = static_cast<int>(x);
            const int yi = static_cast<int>(y);
            if (!is_valid(xi, yi)) [[unlikely]] {
                throw std::invalid_argument("Invalid coordinates for Position");
            }
            return coords_to_index(xi, yi);
        }()} {}
    
    // C++20 string-like constructor with concepts (non-throwing in hard-removal build)
    template <PositionStringLike StringType>
    constexpr Position(const StringType& pos_str)
        requires requires {
            pos_str[0];
            pos_str[1];
        }
        : index_{[&]() {
              const int x = pos_str[0] - 'A';
              const int y = pos_str[1] - '1';
              return is_valid(x, y) ? coords_to_index(x, y) : std::uint8_t{0};
          }()} {}

    // Legacy C-array constructor (non-throwing in hard-removal build)
    constexpr Position(const char pos_str[2])
        : index_{[&]() {
              if (!pos_str) return std::uint8_t{0};
              const int x = pos_str[0] - 'A';
              const int y = pos_str[1] - '1';
              return is_valid(x, y) ? coords_to_index(x, y) : std::uint8_t{0};
          }()} {}

    // Getters with nodiscard and noexcept specifiers
    [[nodiscard]] constexpr int x() const noexcept { return index_to_x(index_); }
    [[nodiscard]] constexpr int y() const noexcept { return index_to_y(index_); }

    // C++20 constexpr improvements
    [[nodiscard]] constexpr std::size_t hash() const noexcept { return static_cast<std::size_t>(index_); }

    [[nodiscard]] constexpr bool is_valid() const noexcept { return index_ < max_positions() && is_valid(x(), y()); }

    // Modern string formatting with std::format (C++20)
    [[nodiscard]] std::string to_string() const noexcept;

    // C++20 spaceship operator with constexpr
    [[nodiscard]] constexpr std::strong_ordering operator<=>(const Position& other) const noexcept {
        return hash() <=> other.hash();
    }

    // Defaulted equality with C++20 improvements
    [[nodiscard]] constexpr bool operator==(const Position& other) const noexcept = default;

    // C++20 range support - get all valid positions
    [[nodiscard]] static constexpr auto all_valid_positions() noexcept {
        std::array<Position, max_positions()> positions{};
        std::size_t index = 0;

        for (int y = 0; y < static_cast<int>(board_size); ++y) {
            for (int x = 0; x < static_cast<int>(board_size); ++x) {
                if (is_valid(x, y) && index < max_positions()) { positions[index++] = Position{x, y}; }
            }
        }
        return positions;
    }

    // C++20 constexpr conversion to coordinate pair
    [[nodiscard]] constexpr CoordinatePair to_pair() const noexcept { return CoordinatePair{.x = x(), .y = y()}; }
};

// C++20 improved std::hash specialization
namespace std {
template <> struct hash<Position> {
    [[nodiscard]] constexpr std::size_t operator()(const Position& p) const noexcept { return p.hash(); }
};

// C++20 format support
template <> struct formatter<Position> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    auto format(const Position& pos, format_context& ctx) const { return format_to(ctx.out(), "{}", pos.to_string()); }
};
} // namespace std
