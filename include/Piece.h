#pragma once
#include <cstdint>
#include <compare>

// Unified piece definitions for the whole project

enum class PieceColor : uint8_t {
    WHITE = 0,
    BLACK = 1
};

enum class PieceType : uint8_t {
    PION = 0,
    DAME = 1
};

struct PieceInfo {
    PieceColor color;
    PieceType type;

    constexpr std::strong_ordering operator<=>(const PieceInfo&) const = default;
};
