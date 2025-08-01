#pragma once
#include <cstdint>
#include <compare>
#include <concepts>
#include <string_view>
#include <type_traits>

// Unified piece definitions for the whole project

enum class PieceColor : uint8_t {
    WHITE = 0,
    BLACK = 1
};

enum class PieceType : uint8_t {
    PION = 0,
    DAME = 1
};

// C++20 concepts for type safety
template<typename T>
concept PieceEnumType = std::same_as<T, PieceColor> || std::same_as<T, PieceType>;

// Consteval utility functions for compile-time evaluation
consteval auto piece_color_count() noexcept -> std::size_t {
    return 2;
}

consteval auto piece_type_count() noexcept -> std::size_t {
    return 2;
}

// Constexpr utility functions with modern C++20 features
constexpr auto to_string(PieceColor color) noexcept -> std::string_view {
    switch (color) {
        case PieceColor::WHITE: return "WHITE";
        case PieceColor::BLACK: return "BLACK";
    }
    return "UNKNOWN";
}

constexpr auto to_string(PieceType type) noexcept -> std::string_view {
    switch (type) {
        case PieceType::PION: return "PION";
        case PieceType::DAME: return "DAME";
    }
    return "UNKNOWN";
}

// Type-safe enum utilities with concepts
template<PieceEnumType T>
constexpr auto to_underlying(T value) noexcept -> std::underlying_type_t<T> {
    return static_cast<std::underlying_type_t<T>>(value);
}

// Modern C++20 struct with enhanced features (maintaining aggregate type)
struct PieceInfo {
    PieceColor color{};
    PieceType type{};

    // Three-way comparison operator (C++20)
    constexpr std::strong_ordering operator<=>(const PieceInfo&) const noexcept = default;
    
    // Equality operator (automatically generated from <=> in C++20)
    constexpr bool operator==(const PieceInfo&) const noexcept = default;

    // Utility methods with modern C++20 features
    [[nodiscard]] constexpr auto is_white() const noexcept -> bool {
        return color == PieceColor::WHITE;
    }

    [[nodiscard]] constexpr auto is_black() const noexcept -> bool {
        return color == PieceColor::BLACK;
    }

    [[nodiscard]] constexpr auto is_pion() const noexcept -> bool {
        return type == PieceType::PION;
    }

    [[nodiscard]] constexpr auto is_dame() const noexcept -> bool {
        return type == PieceType::DAME;
    }

    // String representation
    [[nodiscard]] constexpr auto color_string() const noexcept -> std::string_view {
        return to_string(color);
    }

    [[nodiscard]] constexpr auto type_string() const noexcept -> std::string_view {
        return to_string(type);
    }

    // Hash support for modern containers
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t {
        return static_cast<std::size_t>(to_underlying(color)) << 8 | 
               static_cast<std::size_t>(to_underlying(type));
    }
};

// C++20 concepts for PieceInfo validation
template<typename T>
concept ValidPieceInfo = std::same_as<T, PieceInfo> && 
                        requires(T piece) {
                            { piece.color } -> std::convertible_to<PieceColor>;
                            { piece.type } -> std::convertible_to<PieceType>;
                            { piece.is_white() } -> std::same_as<bool>;
                            { piece.is_black() } -> std::same_as<bool>;
                            { piece.is_pion() } -> std::same_as<bool>;
                            { piece.is_dame() } -> std::same_as<bool>;
                        };

// Static assertions for compile-time validation
static_assert(std::is_aggregate_v<PieceInfo>, "PieceInfo must remain an aggregate type");
static_assert(std::is_trivially_copyable_v<PieceInfo>);
static_assert(std::is_standard_layout_v<PieceInfo>);
static_assert(sizeof(PieceInfo) == 2); // Ensure compact representation
