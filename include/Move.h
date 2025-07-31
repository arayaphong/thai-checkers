#pragma once

#include <optional>
#include <cstdint>
#include <stdexcept>
#include <format>
#include <compare>
#include "Position.h"

struct Move
{
    std::uint8_t depth;
    Position focus;
    std::optional<Position> target;
    std::optional<Position> captured;
    
    // Helper methods with designated initializers
    static constexpr Move make_capture(std::uint8_t depth, const Position& from, 
                                      const Position& to, const Position& captured) noexcept {
        return {.depth = depth, .focus = from, .target = to, .captured = captured};
    }
    
    static constexpr Move make_move(std::uint8_t depth, const Position& from, 
                                   const Position& to) noexcept {
        return {.depth = depth, .focus = from, .target = to, .captured = std::nullopt};
    }
    
    [[nodiscard]] constexpr bool is_capture() const noexcept { 
        return captured.has_value(); 
    }
    [[nodiscard]] static Move from_hash(const std::size_t hash_value);
    [[nodiscard]] constexpr std::strong_ordering operator<=>(const Move& other) const noexcept {
        return hash() <=> other.hash();
    }
    [[nodiscard]] constexpr bool operator==(const Move& other) const noexcept = default;
    [[nodiscard]] constexpr std::size_t hash() const {
        if (!target.has_value()) {
            throw std::invalid_argument("Invalid Move: No valid position");
        }
        
        const bool has_captured = captured.has_value();
        std::size_t hash_value = has_captured ? 1 : 0;
        hash_value = (hash_value << 4) | static_cast<std::size_t>(depth);
        hash_value = (hash_value << 5) | focus.hash();
        hash_value = (hash_value << 5) | target->hash();
        hash_value = (hash_value << 5) | (has_captured ? captured->hash() : 0);
        return hash_value;
    }
};

namespace std {
    template <>
    struct hash<Move> {
        std::size_t operator()(const Move& move) const noexcept {
            return move.hash();
        }
    };
}
