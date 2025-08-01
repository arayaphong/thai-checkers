#include "Options.h"
#include <format>
#include <string_view>

namespace {
    constexpr std::string_view POSITION_TYPE = "Position";
    constexpr std::string_view CAPTURE_TYPE = "CaptureSequence";
}

// Modern C++20 implementation focusing on complex operations that benefit from separate compilation
// Uses modules-like organization and consteval for compile-time computation where possible

namespace Options_impl {
    
    /**
     * @brief Validates capture sequence integrity using C++20 concepts and ranges
     * @param sequence The capture sequence to validate
     * @return True if sequence is valid (even indices represent captured pieces)
     */
    [[nodiscard]] constexpr bool validate_capture_sequence(const CaptureSequence& sequence) noexcept {
        if (sequence.empty()) return false;
        
        // Use C++20 ranges to check sequence integrity
        auto indices = std::views::iota(std::size_t{0}, sequence.size());
        return std::ranges::all_of(indices, [&sequence](const auto i) {
            return i == 0 || sequence[i] != sequence[i-1]; // No consecutive duplicates
        });
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
