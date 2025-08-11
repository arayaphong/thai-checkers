#include "Legals.h"
#include <format>
#include <string_view>

namespace {
constexpr std::string_view POSITION_TYPE = "Position";
constexpr std::string_view CAPTURE_TYPE = "CaptureSequence";
} // namespace

namespace Legals_impl {
[[nodiscard]] constexpr bool validate_move_info(const MoveInfo& move_info) noexcept {
    if (!move_info.target_position.is_valid()) return false;
    return std::ranges::all_of(move_info.captured_positions, [](const Position& pos) { return pos.is_valid(); });
}

[[nodiscard]] std::string format_legals_info(const Legals& legals) {
    const auto type_name = legals.has_captured() ? CAPTURE_TYPE : POSITION_TYPE;
    return std::format("Legals[type: {}, count: {}]", type_name, legals.size());
}
} // namespace Legals_impl
