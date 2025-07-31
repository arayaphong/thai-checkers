#pragma once

#include <vector>
#include <set>
#include <variant>
#include <stdexcept>
#include <concepts>
#include <ranges>
#include <iterator>
#include <type_traits>

#include "Position.h"

// Type aliases for better readability
using Positions = std::vector<Position>;
using CaptureSequence = std::vector<Position>;
using CaptureSequences = std::set<CaptureSequence>;

// C++20 concepts for type safety
template<typename T>
concept IndexType = std::integral<T> && std::is_convertible_v<T, std::size_t>;

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

public:
    /**
     * @brief Constructs Options with regular positions.
     * @param pos Vector of positions for regular moves.
     */
    explicit Options(Positions pos) : move_data(std::move(pos)) {}

    /**
     * @brief Constructs Options with capture sequences.
     * @param seq Set of capture sequences.
     */
    explicit Options(CaptureSequences seq) : move_data(std::move(seq)) {}

    /**
     * @brief Checks if there are any captured pieces in the move data.
     * @return True if there are captured pieces, false otherwise.
     */
    [[nodiscard]] constexpr bool has_captured() const noexcept {
        return std::holds_alternative<CaptureSequences>(move_data);
    }

    /**
     * @brief Checks if this contains regular positions.
     * @return True if contains positions, false otherwise.
     */
    [[nodiscard]] constexpr bool has_positions() const noexcept {
        return std::holds_alternative<Positions>(move_data);
    }

    /**
     * @brief Gets the regular positions, returns empty vector if none available.
     * @return Vector of positions.
     */
    [[nodiscard]] const Positions& get_positions() const {
        if (const auto* pos = std::get_if<Positions>(&move_data)) {
            return *pos;
        }
        static const Positions empty{};
        return empty;
    }

    /**
     * @brief Gets the capture sequences, returns empty set if none available.
     * @return Set of capture sequences.
     */
    [[nodiscard]] const CaptureSequences& get_capture_sequences() const {
        if (const auto* seqs = std::get_if<CaptureSequences>(&move_data)) {
            return *seqs;
        }
        static const CaptureSequences empty{};
        return empty;
    }

    /**
     * @brief Gets the number of available moves/sequences.
     * @return Size of the underlying container.
     */
    [[nodiscard]] std::size_t size() const noexcept {
        return std::visit([](const auto& container) { return container.size(); }, move_data);
    }

    /**
     * @brief Checks if there are no moves available.
     * @return True if empty, false otherwise.
     */
    [[nodiscard]] constexpr bool empty() const noexcept {
        return std::visit([](const auto& container) { return container.empty(); }, move_data);
    }

    /**
     * @brief Gets a specific position by index with bounds checking.
     * @tparam IndexT Index type (must be integral)
     * @param index Index of the position to retrieve.
     * @return Position at the given index.
     * @throws std::out_of_range if index is invalid.
     * @throws std::invalid_argument if no positions are available.
     */
    template<IndexType IndexT = std::size_t>
    [[nodiscard]] Position get_position(IndexT index) const {
        const auto idx = static_cast<std::size_t>(index);
        
        if (const auto* positions = std::get_if<Positions>(&move_data)) {
            if (idx >= positions->size()) {
                throw std::out_of_range("Index out of range for positions");
            }
            return (*positions)[idx];
        } 
        
        if (const auto* sequences = std::get_if<CaptureSequences>(&move_data)) {
            if (idx >= sequences->size()) {
                throw std::out_of_range("Index out of range for capture sequences");
            }
            auto it = std::ranges::next(sequences->begin(), idx);
            return it->back(); // Get the last position in the capture sequence
        }
        
        throw std::invalid_argument("No position available");
    }

    /**
     * @brief Gets a specific capture sequence by index with bounds checking.
     * @tparam IndexT Index type (must be integral)
     * @param index Index of the capture sequence to retrieve.
     * @return Capture sequence at the given index.
     * @throws std::out_of_range if index is invalid.
     * @throws std::invalid_argument if no capture sequences are available.
     */
    template<IndexType IndexT = std::size_t>
    [[nodiscard]] const CaptureSequence& get_capture_sequence(IndexT index) const {
        const auto idx = static_cast<std::size_t>(index);
        
        if (const auto* sequences = std::get_if<CaptureSequences>(&move_data)) {
            if (idx >= sequences->size()) {
                throw std::out_of_range("Index out of range for capture sequences");
            }
            auto it = std::ranges::next(sequences->begin(), idx);
            return *it;
        }
        
        throw std::invalid_argument("No capture sequences available");
    }

    /**
     * @brief Gets the positions of captured pieces for a specific capture sequence.
     * @tparam IndexT Index type (must be integral)
     * @param index Index of the capture sequence.
     * @return Vector of positions representing captured pieces.
     * @throws std::out_of_range if index is invalid.
     * @throws std::invalid_argument if no capture sequences are available.
     */
    template<IndexType IndexT = std::size_t>
    [[nodiscard]] Positions get_capture_pieces(IndexT index) const {
        const auto& sequence = get_capture_sequence(index);
        
        Positions captured_positions;
        // In CaptureSequence: [start, captured1, landing1, captured2, landing2, ..., end]
        // Captured pieces are at odd indices (1, 3, ...), landing positions at even indices (2, 4, ...)
        // We collect all captured pieces by iterating odd indices, skipping the first and last positions.
        
        // Use C++20 ranges for more expressive code
        auto captured_indices = std::views::iota(std::size_t{1}, sequence.size() - 1) 
                              | std::views::filter([](std::size_t i) { return i % 2 == 1; });
        
        for (const auto i : captured_indices) {
            captured_positions.push_back(sequence[i]);
        }
        
        return captured_positions;
    }

    /**
     * @brief Visit the underlying move data with a callable.
     * @tparam Visitor Callable type
     * @param visitor Function to call with the underlying data
     * @return Result of the visitor function
     */
    template<typename Visitor>
    [[nodiscard]] constexpr auto visit(Visitor&& visitor) const 
        -> decltype(std::visit(std::forward<Visitor>(visitor), move_data)) {
        return std::visit(std::forward<Visitor>(visitor), move_data);
    }
};
