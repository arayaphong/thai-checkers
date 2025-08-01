#pragma once

#include <vector>
#include <set>
#include <cstdint>
#include <concepts>
#include <compare>
#include <utility>

#include "Position.h"

// C++20: Strong enum class with explicit underlying type
enum class AnalyzerDirection : std::uint8_t { 
    NW = 0, 
    NE = 1, 
    SW = 2, 
    SE = 3 
};

// C++20: Concept to ensure AnalyzerDirection is valid
template<typename T>
concept ValidAnalyzerDirection = std::same_as<T, AnalyzerDirection>;

// C++20: Semantic type for board direction deltas with spaceship operator
struct AnalyzerDirectionDelta {
    std::int8_t row{};
    std::int8_t col{};
    
    // C++20: Three-way comparison operator for efficient comparisons
    [[nodiscard]] constexpr auto operator<=>(const AnalyzerDirectionDelta&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const AnalyzerDirectionDelta&) const noexcept = default;
    
    // C++20: Constexpr member functions for compile-time computations
    [[nodiscard]] constexpr bool is_diagonal() const noexcept {
        return (row == col) || (row == -col);
    }
    
    [[nodiscard]] constexpr bool is_forward(bool is_black_piece) const noexcept {
        return is_black_piece ? (row > 0) : (row < 0);
    }
};

// C++20: Semantic type for capture move information with designated initializers support
struct AnalyzerCaptureMove {
    Position captured_piece{};
    Position landing_position{};
    
    // C++20: Three-way comparison for efficient sorting and searching
    [[nodiscard]] constexpr auto operator<=>(const AnalyzerCaptureMove&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const AnalyzerCaptureMove&) const noexcept = default;
    
    // C++20: Constexpr validation
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return captured_piece != landing_position;
    }
};
