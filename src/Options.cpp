#include "Options.h"
#include <format>
#include <string_view>

namespace {
    constexpr std::string_view POSITION_TYPE = "Position";
    constexpr std::string_view CAPTURE_TYPE = "CaptureSequence";
}

// Modern C++20 implementation focusing on utility functions
// Uses modules-like organization and consteval for compile-time computation where possible

namespace Options_impl {
    
    /**
     * @brief Validates move info integrity using C++20 concepts and ranges
     * @param move_info The move info to validate
     * @return True if move info is valid
     */
    [[nodiscard]] constexpr bool validate_move_info(const MoveInfo& move_info) noexcept {
        // Check that target position is valid
        if (!move_info.target_position.is_valid()) return false;
        
        // Check that all captured positions are valid
        return std::ranges::all_of(move_info.captured_positions, 
                                  [](const Position& pos) { return pos.is_valid(); });
    }

    /**
     * @brief Creates a formatted string representation of the options
     * @param options The Options object to format
     * @return String representation using C++20 std::format
     */
    [[nodiscard]] std::string format_options_info(const Options& options) {
        const auto type_name = options.has_captured() ? CAPTURE_TYPE : POSITION_TYPE;
        return std::format("Options[type: {}, count: {}]", type_name, options.size());
    }

} // namespace Options_impl
