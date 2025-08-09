#include "Board.h"
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <ranges>
#include <bitset>
#include <format>
#include <optional>

std::size_t Board::hash() const noexcept {
    const std::bitset<64> hash_bits = [&]() {
        int piece_count = 0;
        std::bitset<64> bits;
        
        std::ranges::for_each(
            std::views::iota(0, 32),
            [&](int i) {
                const auto pos = Position::from_index(i);
                if (const auto it = pieces_.find(pos); it != pieces_.end()) {
                    bits[i + 32] = true;
                    if (piece_count < 16) {
                        const auto& [position, piece_info] = *it;
                        // Color bit (bits 16-31)
                        bits[piece_count + 16] = (piece_info.color == PieceColor::BLACK);
                        // Type bit (bits 0-15)  
                        bits[piece_count] = (piece_info.type == PieceType::DAME);
                        ++piece_count;
                    }
                }
            }
        );
        
        return bits;
    }();

    return hash_bits.to_ullong();
}

Board Board::from_hash(std::size_t hash_value) {
    const std::bitset<64> hash_bits(hash_value);
    Pieces temp_pieces;
    temp_pieces.reserve(Board::PIECES_RESERVE_SIZE);
    int piece_count = 0;
    
    // Modern C++20: Use ranges algorithm instead of manual loop
    std::ranges::for_each(
        std::views::iota(0, 32) 
        | std::views::filter([&hash_bits](int i) { return hash_bits[i + 32]; })
        | std::views::take(16),
        [&](int board_index) {
            const auto pos = Position::from_index(board_index);
            const PieceColor color = hash_bits[piece_count + 16] ? PieceColor::BLACK : PieceColor::WHITE;
            const PieceType type = hash_bits[piece_count] ? PieceType::DAME : PieceType::PION;
            // Use emplace to construct in-place and avoid copies
            temp_pieces.emplace(pos, PieceInfo{.color = color, .type = type});
            ++piece_count;
        }
    );
    
    return Board{std::move(temp_pieces)};
}

Board Board::setup() noexcept {
    Board board;
    board.pieces_.reserve(Board::PIECES_RESERVE_SIZE);

    // Fill black pieces (rows 0-1)
    for (int row = 0; row < 2; ++row) {
        const int start_col = (row % 2 == 0) ? 1 : 0;
        for (int i = 0; i < 4; ++i) {
            const int col = start_col + i * 2;
            // Use emplace to construct in-place
            board.pieces_.emplace(Position::from_coords(col, row), 
                                 PieceInfo{.color = PieceColor::BLACK, .type = PieceType::PION});
        }
    }
    
    // Fill white pieces (rows 6-7)
    for (int row = 6; row < 8; ++row) {
        const int start_col = (row % 2 == 0) ? 1 : 0;
        for (int i = 0; i < 4; ++i) {
            const int col = start_col + i * 2;
            // Use emplace to construct in-place
            board.pieces_.emplace(Position::from_coords(col, row), 
                                 PieceInfo{.color = PieceColor::WHITE, .type = PieceType::PION});
        }
    }
    
    return board;
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

std::optional<PieceColor> Board::get_piece_color(const Position& pos) const noexcept {
    if (!is_valid_position(pos)) [[unlikely]] {
        return std::nullopt;
    }
    
    if (const auto it = pieces_.find(pos); it != pieces_.end()) [[likely]] {
        return it->second.color;
    }
    
    return std::nullopt;
}

std::optional<PieceType> Board::get_piece_type(const Position& pos) const noexcept {
    if (!is_valid_position(pos)) [[unlikely]] {
        return std::nullopt;
    }
    
    if (const auto it = pieces_.find(pos); it != pieces_.end()) [[likely]] {
        return it->second.type;
    }
    
    return std::nullopt;
}

std::optional<PieceInfo> Board::get_piece_info(const Position& pos) const noexcept {
    if (!is_valid_position(pos)) [[unlikely]] {
        return std::nullopt;
    }
    
    if (const auto it = pieces_.find(pos); it != pieces_.end()) [[likely]] {
        return it->second;
    }
    
    return std::nullopt;
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

Pieces Board::get_pieces(PieceColor color) const noexcept {
    Pieces filtered;
    filtered.reserve(Board::PIECES_RESERVE_SIZE / 2); // Estimate half pieces per color
    
    // Use std::ranges::for_each instead of copy_if to reduce intermediate containers
    std::ranges::for_each(pieces_, [&filtered, color](const auto& pair) {
        if (pair.second.color == color) {
            filtered.emplace(pair.first, pair.second);
        }
    });
    
    return filtered;
}

void Board::reset() noexcept {
    pieces_.clear();
    pieces_.reserve(PIECES_RESERVE_SIZE);
}
