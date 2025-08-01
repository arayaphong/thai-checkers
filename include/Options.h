#pragma once

#include <vector>
#include <set>
#include <variant>
#include <stdexcept>
#include <ranges>
#include <iterator>
#include <algorithm>
#include <concepts>
#include <type_traits>

#include "Position.h"

using Positions = std::vector<Position>;
using CaptureSequence = std::vector<Position>;
using CaptureSequences = std::set<CaptureSequence>;

// C++20 concepts for type safety and better error messages
template<typename T>
concept PositionContainer = requires(T t) {
    typename T::value_type;
    requires std::same_as<typename T::value_type, Position>;
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.empty() } -> std::convertible_to<bool>;
};

template<typename T>
concept CaptureContainer = requires(T t) {
    typename T::value_type;
    requires std::same_as<typename T::value_type, CaptureSequence>;
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.empty() } -> std::convertible_to<bool>;
};

/**
 * @brief A wrapper class that can hold either regular move positions or capture sequences.
 * 
 * This class provides a unified interface for accessing move data, whether it's simple
 * position moves or complex capture sequences with multiple captured pieces.
 * Uses C++20 std::variant for type-safe storage and modern C++ features.
 */
class Options {
private:
    std::variant<Positions, CaptureSequences> move_data;

    /**
     * @brief Gets a specific capture sequence by index with bounds checking.
     * @param index Index of the capture sequence to retrieve.
     * @return Capture sequence at the given index.
     * @throws std::out_of_range if index is invalid.
     * @throws std::invalid_argument if no capture sequences are available.
     */
    [[nodiscard]] const CaptureSequence& get_capture_sequence(const std::size_t index) const {
        if (const auto* const sequences = std::get_if<CaptureSequences>(&move_data)) [[likely]] {
            if (index >= sequences->size()) [[unlikely]] {
                throw std::out_of_range("Index out of range for capture sequences");
            }
            const auto it = std::ranges::next(sequences->begin(), static_cast<std::ptrdiff_t>(index));
            return *it;
        }
        
        throw std::invalid_argument("No capture sequences available");
    }

public:
    /**
     * @brief Constructs Options with regular positions using C++20 concepts.
     * @param pos Container of positions for regular moves.
     */
    explicit Options(const PositionContainer auto& pos) 
        requires std::same_as<std::decay_t<decltype(pos)>, Positions>
        : move_data(pos) {}

    /**
     * @brief Constructs Options with capture sequences using C++20 concepts.
     * @param seq Container of capture sequences.
     */
    explicit Options(const CaptureContainer auto& seq) 
        requires std::same_as<std::decay_t<decltype(seq)>, CaptureSequences>
        : move_data(seq) {}

    // Move constructor removed - not needed for facade pattern

    /**
     * @brief Checks if there are any captured pieces in the move data.
     * @return True if there are captured pieces, false otherwise.
     */
    [[nodiscard]] constexpr bool has_captured() const noexcept {
        return std::holds_alternative<CaptureSequences>(move_data);
    }

    /**
     * @brief Gets the number of available moves/sequences using C++20 std::visit.
     * @return Size of the underlying container.
     */
    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return std::visit([]<typename T>(const T& container) constexpr -> std::size_t { 
            return container.size(); 
        }, move_data);
    }

    /**
     * @brief Checks if there are no moves available using C++20 abbreviated function template.
     * @return True if empty, false otherwise.
     */
    [[nodiscard]] constexpr bool empty() const noexcept {
        return std::visit([]<typename T>(const T& container) constexpr { 
            return container.empty(); 
        }, move_data);
    }

    /**
     * @brief Gets a specific position by index with bounds checking using C++20 features.
     * @param index Index of the position to retrieve.
     * @return Position at the given index.
     * @throws std::out_of_range if index is invalid.
     * @throws std::invalid_argument if no positions are available.
     */
    [[nodiscard]] Position get_position(const std::size_t index) const {
        return std::visit([index]<typename T>(const T& container) -> Position {
            if (index >= container.size()) [[unlikely]] {
                throw std::out_of_range("Index out of range");
            }
            if constexpr (std::same_as<T, Positions>) {
                return container[index];
            } else {
                const auto it = std::ranges::next(container.begin(), static_cast<std::ptrdiff_t>(index));
                return it->back(); // Get the last position in the capture sequence
            }
        }, move_data);
    }

    /**
     * @brief Gets the positions of captured pieces for a specific capture sequence using C++20 ranges.
     * @param index Index of the capture sequence.
     * @return Vector of positions representing captured pieces.
     * @throws std::out_of_range if index is invalid.
     * @throws std::invalid_argument if no capture sequences are available.
     */
    [[nodiscard]] Positions get_capture_pieces(const std::size_t index) const {
        const auto& sequence = get_capture_sequence(index);
        
        // Modern C++20 ranges approach with views and algorithms
        auto even_indices = std::views::iota(std::size_t{0}, sequence.size()) 
                          | std::views::filter([](const auto i) { return i % 2 == 0; });
        
        Positions captured_positions;
        captured_positions.reserve(std::ranges::distance(even_indices));
        
        std::ranges::transform(even_indices, std::back_inserter(captured_positions),
                              [&sequence](const auto i) { return sequence[i]; });
        
        return captured_positions;
    }
};
