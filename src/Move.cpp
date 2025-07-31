#include "Move.h"
#include <format>
#include <stdexcept>

Move Move::from_hash(const std::size_t hash_value) {
    // Reverse the bit packing done in hash() method
    // Layout: [has_captured(1)] [depth(4)] [focus_pos(5)] [valid_pos(5)] [captured_pos(5)]
    
    const auto captured_hash = hash_value & 0x1F;           // bits 0-4
    const auto valid_hash = (hash_value >> 5) & 0x1F;       // bits 5-9
    const auto focus_hash = (hash_value >> 10) & 0x1F;      // bits 10-14
    const auto depth = static_cast<std::uint8_t>((hash_value >> 15) & 0x0F); // bits 15-18
    const bool has_captured = (hash_value >> 19) & 0x1;     // bit 19
    
    try {
        return {
            .depth = depth,
            .focus = Position::from_index(static_cast<int>(focus_hash)),
            .target = Position::from_index(static_cast<int>(valid_hash)),
            .captured = has_captured ? 
                std::optional<Position>{Position::from_index(static_cast<int>(captured_hash))} : 
                std::nullopt
        };
    } catch (const std::out_of_range& e) {
        throw std::invalid_argument(std::format("Invalid hash value: contains invalid position indices: {}", e.what()));
    }
}
