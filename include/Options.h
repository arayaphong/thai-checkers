#pragma once

#include <vector>
#include <set>
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
    typename std::remove_cvref_t<T>::value_type;
    requires std::same_as<typename std::remove_cvref_t<T>::value_type, Position>;
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.empty() } -> std::convertible_to<bool>;
};

/**
 * @brief Simplified move data structure that digests input at construction time.
 * 
 * This struct holds the essential data extracted from either regular positions
 * or capture sequences, providing a uniform interface for accessing move information.
 */
struct MoveInfo {
    Position target_position;           // Where the piece lands
    Positions captured_positions;       // What pieces are captured (empty for regular moves)
    
    // C++20 three-way comparison for sorting and searching
    [[nodiscard]] constexpr auto operator<=>(const MoveInfo&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const MoveInfo&) const noexcept = default;
    
    // Check if this is a capture move
    [[nodiscard]] constexpr bool is_capture() const noexcept {
        return !captured_positions.empty();
    }
};

/**
 * @brief Simplified wrapper class that digests input data at construction and stores only essential information.
 * 
 * This class processes either regular move positions or capture sequences at construction time
 * and stores them as a uniform vector of MoveInfo structures. This eliminates the need for
 * complex variant handling and provides a cleaner, more efficient interface.
 * Uses C++20 features for modern, efficient implementation.
 */
class Options {
private:
    std::vector<MoveInfo> moves_;
    bool has_captures_;

    /**
     * @brief Processes a capture sequence and extracts target position and captured pieces.
     * @param sequence The capture sequence to process.
     * @return MoveInfo with target position and captured pieces.
     */
    [[nodiscard]] static MoveInfo process_capture_sequence(const CaptureSequence& sequence) {      
        MoveInfo move_info;
        move_info.target_position = sequence.back(); // Last position is the landing position
        
        // Extract captured pieces (even indices in the sequence)
        auto even_indices = std::views::iota(std::size_t{0}, sequence.size()) 
                          | std::views::filter([](const auto i) { return i % 2 == 0; });
        
        move_info.captured_positions.reserve(std::ranges::distance(even_indices));
        std::ranges::transform(even_indices, std::back_inserter(move_info.captured_positions),
                              [&sequence](const auto i) -> const Position& { return sequence[i]; });
        
        return move_info;
    }

public:
    /**
     * @brief Constructs Options with regular positions.
     * @param positions Container of positions for regular moves.
     */
    explicit Options(const Positions& positions) : has_captures_(false) {
        moves_.reserve(positions.size());
        std::ranges::transform(positions, std::back_inserter(moves_),
                              [](const Position& pos) -> MoveInfo {
                                  return MoveInfo{.target_position = pos, .captured_positions = {}};
                              });
    }

    /**
     * @brief Constructs Options with regular positions using move semantics.
     * @param positions Container of positions for regular moves.
     */
    explicit Options(Positions&& positions) : has_captures_(false) {
        moves_.reserve(positions.size());
        for (auto&& pos : positions) {
            moves_.emplace_back(MoveInfo{.target_position = std::move(pos), .captured_positions = {}});
        }
    }

    /**
     * @brief Constructs Options with capture sequences, processing them immediately.
     * @param sequences Container of capture sequences.
     */
    explicit Options(const CaptureSequences& sequences) : has_captures_(true) {
        moves_.reserve(sequences.size());
        std::ranges::transform(sequences, std::back_inserter(moves_),
                              [](const CaptureSequence& seq) -> MoveInfo {
                                  return process_capture_sequence(seq);
                              });
    }

    /**
     * @brief Constructs Options with capture sequences using move semantics, processing them immediately.
     * @param sequences Container of capture sequences.
     */
    explicit Options(CaptureSequences&& sequences) : has_captures_(true) {
        moves_.reserve(sequences.size());
        for (auto&& seq : sequences) {
            moves_.emplace_back(process_capture_sequence(seq));
        }
    }

    /**
     * @brief Checks if there are any captured pieces in the move data.
     * @return True if there are captured pieces, false otherwise.
     */
    [[nodiscard]] constexpr bool has_captured() const noexcept {
        return has_captures_;
    }

    /**
     * @brief Gets the number of available moves.
     * @return Size of the moves container.
     */
    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return moves_.size();
    }

    /**
     * @brief Checks if there are no moves available.
     * @return True if empty, false otherwise.
     */
    [[nodiscard]] constexpr bool empty() const noexcept {
        return moves_.empty();
    }

    /**
     * @brief Gets a specific position by index with bounds checking.
     * @param index Index of the position to retrieve.
     * @return Position at the given index.
     * @throws std::out_of_range if index is invalid.
     */
    [[nodiscard]] const Position& get_position(const std::size_t index) const {
        const std::size_t i = moves_.empty() ? 0 : std::min(index, moves_.size() - 1);
        static const Position kDefault{};
        return moves_.empty() ? kDefault : moves_[i].target_position;
    }

    /**
     * @brief Gets the positions of captured pieces for a specific move.
     * @param index Index of the move.
     * @return Vector of positions representing captured pieces (empty for regular moves).
     * @throws std::out_of_range if index is invalid.
     * @throws std::invalid_argument if this was constructed from regular positions (for backward compatibility).
     */
    [[nodiscard]] const Positions& get_capture_pieces(const std::size_t index) const {
        static const Positions kEmpty;
        const std::size_t i = std::min(index, moves_.size() - 1);
        return moves_[i].captured_positions;
    }

    /**
     * @brief Gets complete move information by index.
     * @param index Index of the move.
     * @return MoveInfo containing target position and captured pieces.
     * @throws std::out_of_range if index is invalid.
     */
    [[nodiscard]] const MoveInfo& get_move_info(const std::size_t index) const {
        static const MoveInfo kDefault{};
        if (moves_.empty()) return kDefault;
        const std::size_t i = std::min(index, moves_.size() - 1);
        return moves_[i];
    }

    /**
     * @brief Provides range-based iteration over moves.
     * @return Iterator to the beginning of moves.
     */
    [[nodiscard]] auto begin() const noexcept { return moves_.begin(); }
    
    /**
     * @brief Provides range-based iteration over moves.
     * @return Iterator to the end of moves.
     */
    [[nodiscard]] auto end() const noexcept { return moves_.end(); }

    /**
     * @brief Provides range-based iteration over moves.
     * @return Const iterator to the beginning of moves.
     */
    [[nodiscard]] auto cbegin() const noexcept { return moves_.cbegin(); }
    
    /**
     * @brief Provides range-based iteration over moves.
     * @return Const iterator to the end of moves.
     */
    [[nodiscard]] auto cend() const noexcept { return moves_.cend(); }
};
