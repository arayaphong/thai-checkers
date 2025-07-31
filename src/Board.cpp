#include "Board.h"
#include <stdexcept>
#include <utility>
#include <algorithm> // for std::count_if
#include <bitset>

Board::Board() {}

Board::~Board() = default;

Board::Board(const Board &other) : pieces(other.pieces) {}

Board& Board::operator=(const Board& other) {
    if (this != &other) {
        pieces = other.pieces;
    }
    return *this;
}
Board::Board(Board&& other) noexcept = default;
Board::Board(const Pieces &pieces) : pieces(pieces) {}

Board &Board::operator=(Board &&other) noexcept = default;

std::size_t Board::hash() const noexcept {
    // A 64-bit hash value composed of:
    // - 32 bits for piece occupancy on the 32 valid squares.
    // - 16 bits for the color of each piece (up to 16 pieces).
    // - 16 bits for the type (dame/king) of each piece (up to 16 pieces).
    // This hashing scheme is order-dependent and assumes a maximum of 16 pieces
    // will be encoded for color and type based on their position index.

    std::bitset<64> hash_bits;
    int piece_count = 0;

    // Iterate through all 32 valid board squares to build the hash.
    for (int i = 0; i < 32; ++i) {
        const auto pos = Position::from_index(i);
        if (const auto it = pieces.find(pos); it != pieces.end()) {
            // Set occupancy bit (stored in the upper 32 bits of the hash).
            hash_bits[i + 32] = true;

            // Encode piece color and type for the first 16 pieces found.
            if (piece_count < 16) {
                const auto& piece = it->second;
                // Color bit (stored in bits 16-31)
                hash_bits[piece_count + 16] = (piece.color == PieceColor::BLACK);
                // Type bit (stored in bits 0-15)
                hash_bits[piece_count] = (piece.type == PieceType::DAME);
                piece_count++;
            }
        }
    }

    return static_cast<std::size_t>(hash_bits.to_ullong());
}

Board Board::from_hash(std::size_t hash_value) {
    Board board;
    const std::bitset<64> hash_bits(hash_value);
    int piece_count = 0;

    // Reconstruct the pieces based on the occupancy bits.
    for (int i = 0; i < 32; ++i) {
        // Check occupancy bit from the upper 32 bits.
        if (hash_bits[i + 32]) {
            const auto pos = Position::from_index(i);
            // Color from bits 16-31
            const auto color = hash_bits[piece_count + 16] ? PieceColor::BLACK : PieceColor::WHITE;
            // Type from bits 0-15
            const auto type = hash_bits[piece_count] ? PieceType::DAME : PieceType::PION;
            
            board.pieces.emplace(pos, PieceInfo{color, type});
            piece_count++;
        }
    }

    return board;
}

bool Board::is_valid_position(const Position& pos) {
    return Position::is_valid(pos.x, pos.y) && ((pos.x + pos.y) % 2 != 0);
}

bool Board::is_occupied(const Position& pos) const {
    return is_valid_position(pos) && pieces.contains(pos);
}

bool Board::is_black_piece(const Position& pos) const {
    if (!is_valid_position(pos)) throw std::invalid_argument("Invalid position");
    auto it = pieces.find(pos);
    if (it == pieces.end()) throw std::invalid_argument("No piece at position");
    return it->second.color == PieceColor::BLACK;
}

bool Board::is_dame_piece(const Position& pos) const {
    if (!is_valid_position(pos)) throw std::invalid_argument("Invalid position");
    auto it = pieces.find(pos);
    if (it == pieces.end()) throw std::invalid_argument("No piece at position");
    return it->second.type == PieceType::DAME;
}

void Board::promote_piece(const Position& pos) {
    if (!is_valid_position(pos)) throw std::invalid_argument("Invalid position");
    auto it = pieces.find(pos);
    if (it == pieces.end()) throw std::invalid_argument("No piece at position");
    if (it->second.type == PieceType::DAME) return; // Already dame
    pieces[pos] = PieceInfo{it->second.color, PieceType::DAME};
}

void Board::move_piece(const Position& from, const Position& to) {
    if (!is_valid_position(from) || !is_valid_position(to)) throw std::invalid_argument("Invalid position");
    if (from == to) return; // No move needed
    if (!is_occupied(from)) throw std::invalid_argument("No piece at from position");
    if (is_occupied(to)) throw std::invalid_argument("To position already occupied");

    pieces[to] = pieces[from];
    pieces.erase(from);
}

void Board::remove_piece(const Position& pos) {
    if (!is_valid_position(pos)) throw std::invalid_argument("Invalid position");
    pieces.erase(pos);
}

int Board::get_piece_count(PieceColor color) const {
    return static_cast<int>(std::count_if(pieces.begin(), pieces.end(),
        [color](const auto& pair) {
            return pair.second.color == color;
        }));
}

int Board::get_piece_count() const {
    return static_cast<int>(pieces.size());
}

void Board::reset() {
    pieces.clear();
}
