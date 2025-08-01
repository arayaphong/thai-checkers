#include "Board.h"
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <ranges>
#include <bitset>
#include <format>

std::size_t Board::hash() const noexcept {
    // Modern C++20 compatible reversible hash for up to 16 pieces
    // Uses bitset for efficient storage and reconstruction
    std::bitset<64> hash_bits;
    int piece_count = 0;

    // Process all 32 possible board positions (8x8 checkerboard = 32 valid squares)
    for (int i = 0; i < 32; ++i) {
        const auto pos = Position::from_index(i);
        if (const auto it = pieces_.find(pos); it != pieces_.end()) {
            // Set occupancy bit (upper 32 bits)
            hash_bits[i + 32] = true;

            // Store piece info for first 16 pieces (C++20 structured binding)
            if (piece_count < 16) {
                const auto& [position, piece_info] = *it;
                // Color bit (bits 16-31)
                hash_bits[piece_count + 16] = (piece_info.color == PieceColor::BLACK);
                // Type bit (bits 0-15)  
                hash_bits[piece_count] = (piece_info.type == PieceType::DAME);
                ++piece_count;
            }
        }
    }

    return hash_bits.to_ullong();
}

Board Board::from_hash(std::size_t hash_value) {
    Board board;
    const std::bitset<64> hash_bits{hash_value};
    int piece_count = 0;

    // Modern C++20: Reconstruct pieces from hash bits
    for (int i = 0; i < 32; ++i) {
        // Check occupancy bit (upper 32 bits)
        if (hash_bits[i + 32]) {
            const auto pos = Position::from_index(i);
            
            // Decode piece info using designated initializers (C++20)
            const auto color = hash_bits[piece_count + 16] ? 
                PieceColor::BLACK : PieceColor::WHITE;
            const auto type = hash_bits[piece_count] ? 
                PieceType::DAME : PieceType::PION;
            
            board.pieces_.emplace(pos, PieceInfo{
                .color = color,
                .type = type
            });
            ++piece_count;
        }
    }

    return board;
}

constexpr bool Board::is_valid_position(const Position& pos) noexcept {
    return Position::is_valid(pos.x(), pos.y()) && ((pos.x() + pos.y()) % 2 != 0);
}

bool Board::is_occupied(const Position& pos) const noexcept {
    return is_valid_position(pos) && pieces_.contains(pos);
}

bool Board::is_black_piece(const Position& pos) const {
    if (!is_valid_position(pos)) [[unlikely]] {
        throw std::invalid_argument("Invalid position");
    }
    
    if (const auto it = pieces_.find(pos); it != pieces_.end()) [[likely]] {
        return it->second.color == PieceColor::BLACK;
    }
    
    throw std::invalid_argument("No piece at position");
}

bool Board::is_dame_piece(const Position& pos) const {
    if (!is_valid_position(pos)) [[unlikely]] {
        throw std::invalid_argument("Invalid position");
    }
    
    if (const auto it = pieces_.find(pos); it != pieces_.end()) [[likely]] {
        return it->second.type == PieceType::DAME;
    }
    
    throw std::invalid_argument("No piece at position");
}

void Board::promote_piece(const Position& pos) {
    if (!is_valid_position(pos)) [[unlikely]] {
        throw std::invalid_argument("Invalid position");
    }
    
    auto it = pieces_.find(pos);
    if (it == pieces_.end()) [[unlikely]] {
        throw std::invalid_argument("No piece at position");
    }
    
    if (it->second.type == PieceType::DAME) [[unlikely]] {
        return; // Already promoted
    }
    
    // Use structured binding and designated initializer
    pieces_[pos] = PieceInfo{.color = it->second.color, .type = PieceType::DAME};
}

void Board::move_piece(const Position& from, const Position& to) {
    if (!is_valid_position(from) || !is_valid_position(to)) [[unlikely]] {
        throw std::invalid_argument("Invalid position");
    }
    
    if (from == to) [[unlikely]] {
        return; // No move needed
    }
    
    if (!is_occupied(from)) [[unlikely]] {
        throw std::invalid_argument("No piece at from position");
    }
    
    if (is_occupied(to)) [[unlikely]] {
        throw std::invalid_argument("To position already occupied");
    }

    // Extract and move the piece using modern C++20 features
    if (auto node = pieces_.extract(from); !node.empty()) [[likely]] {
        node.key() = to;
        pieces_.insert(std::move(node));
    }
}

void Board::remove_piece(const Position& pos) noexcept {
    if (is_valid_position(pos)) [[likely]] {
        pieces_.erase(pos);
    }
}

int Board::get_piece_count(PieceColor color) const noexcept {
    // Use modern C++20 ranges with projection
    return static_cast<int>(std::ranges::count_if(
        pieces_, 
        [color](const PieceInfo& piece) { return piece.color == color; },
        &Pieces::value_type::second
    ));
}

int Board::get_piece_count() const noexcept {
    return static_cast<int>(pieces_.size());
}

void Board::reset() noexcept {
    pieces_.clear();
}
