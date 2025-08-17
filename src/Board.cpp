#include "Board.h"
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <ranges>
#include <bitset>
#include <format>
#include <optional>

Board Board::setup() noexcept {
    Board board;
    // Black rows 0..1
    for (int row = 0; row < 2; ++row) {
        const int start_col = (row % 2 == 0) ? 1 : 0;
        for (int i = 0; i < 4; ++i) {
            const int col = start_col + i * 2;
            const auto p = Position::from_coords(col, row);
            const auto idx = static_cast<unsigned>(p.hash());
            const auto m = static_cast<std::uint32_t>(1u) << idx;
            board.occ_bits_ |= m;
            board.black_bits_ |= m;
            board.dame_bits_ &= ~m;
        }
    }
    // White rows 6..7
    for (int row = 6; row < 8; ++row) {
        const int start_col = (row % 2 == 0) ? 1 : 0;
        for (int i = 0; i < 4; ++i) {
            const int col = start_col + i * 2;
            const auto p = Position::from_coords(col, row);
            const auto idx = static_cast<unsigned>(p.hash());
            const auto m = static_cast<std::uint32_t>(1u) << idx;
            board.occ_bits_ |= m;
            board.black_bits_ &= ~m;
            board.dame_bits_ &= ~m;
        }
    }
    return board;
}

Board Board::from_hash(std::size_t hash) {
    Board board;

    std::bitset<64> bits(hash);
    // Occupancy in bits 32..63
    board.occ_bits_ = static_cast<std::uint32_t>(hash >> 32);

    // Reconstruct dame_bits_ and black_bits_ from first 16 occupied squares' metadata
    board.dame_bits_ = 0;
    board.black_bits_ = 0;
    int count = 0;
    for (int i = 0; i < 32 && count < 16; ++i) {
        const auto m = static_cast<std::uint32_t>(1u) << i;
        if ((board.occ_bits_ & m) == 0u) continue;

        // Dame info is in bits[count], black info is in bits[count + 16]
        if (bits[count]) board.dame_bits_ |= m;
        if (bits[count + 16]) board.black_bits_ |= m;
        ++count;
    }

    return board;
}

std::size_t Board::hash() const noexcept {
    // Compose a 64-bit hash: top 32 bits are occupancy layout, lower bits summarize first up-to-16 pieces' color/type.
    std::bitset<64> bits;
    // Occupancy in bits 32..63
    for (int i = 0; i < 32; ++i) { bits[i + 32] = ((occ_bits_ >> i) & 1u) != 0u; }
    // Encode first 16 occupied squares' metadata (type in 0..15, color in 16..31)
    int count = 0;
    for (int i = 0; i < 32 && count < 16; ++i) {
        const auto m = static_cast<std::uint32_t>(1u) << i;
        if ((occ_bits_ & m) == 0u) continue;
        bits[count] = ((dame_bits_ & m) != 0u);
        bits[count + 16] = ((black_bits_ & m) != 0u);
        ++count;
    }
    return bits.to_ullong();
}

bool Board::is_occupied(const Position& pos) const noexcept {
    if (!is_valid_position(pos)) return false;
    const auto idx = static_cast<unsigned>(pos.hash());
    return (occ_bits_ & (static_cast<std::uint32_t>(1u) << idx)) != 0u;
}

bool Board::is_black_piece(const Position& pos) const noexcept {
    const auto idx = static_cast<unsigned>(pos.hash());
    const auto m = static_cast<std::uint32_t>(1u) << idx;
    return (black_bits_ & m) != 0u;
}

bool Board::is_dame_piece(const Position& pos) const noexcept {
    const auto idx = static_cast<unsigned>(pos.hash());
    const auto m = static_cast<std::uint32_t>(1u) << idx;
    return (dame_bits_ & m) != 0u;
}

void Board::promote_piece(const Position& pos) noexcept {
    if (!is_valid_position(pos)) return;
    const auto idx = static_cast<unsigned>(pos.hash());
    const auto m = static_cast<std::uint32_t>(1u) << idx;
    if ((occ_bits_ & m) == 0u) return;
    dame_bits_ |= m;
}

void Board::move_piece(const Position& from, const Position& to) noexcept {
    const auto fi = static_cast<unsigned>(from.hash());
    const auto ti = static_cast<unsigned>(to.hash());
    const auto fm = static_cast<std::uint32_t>(1u) << fi;
    const auto tm = static_cast<std::uint32_t>(1u) << ti;
    // Only move if from occupied and to empty
    if ((occ_bits_ & fm) == 0u) return;
    if ((occ_bits_ & tm) != 0u) return;
    const bool was_black = (black_bits_ & fm) != 0u;
    const bool was_dame = (dame_bits_ & fm) != 0u;
    // Clear from
    occ_bits_ &= ~fm;
    black_bits_ &= ~fm;
    dame_bits_ &= ~fm;
    // Set to
    occ_bits_ |= tm;
    if (was_black) black_bits_ |= tm;
    else black_bits_ &= ~tm;
    if (was_dame) dame_bits_ |= tm;
    else dame_bits_ &= ~tm;
}

void Board::remove_piece(const Position& pos) noexcept {
    if (!is_valid_position(pos)) return;
    const auto idx = static_cast<unsigned>(pos.hash());
    const auto m = static_cast<std::uint32_t>(1u) << idx;
    occ_bits_ &= ~m;
    black_bits_ &= ~m;
    dame_bits_ &= ~m;
}

Pieces Board::get_pieces(PieceColor color) const noexcept {
    Pieces out;
    out.reserve(12);
    for (int i = 0; i < 32; ++i) {
        const auto m = static_cast<std::uint32_t>(1u) << i;
        if ((occ_bits_ & m) == 0u) continue;
        const bool is_black = (black_bits_ & m) != 0u;
        const bool is_dame = (dame_bits_ & m) != 0u;
        if ((color == PieceColor::BLACK) != is_black) continue;
        const auto pos = Position::from_index(i);
        out.emplace(pos, PieceInfo{.color = is_black ? PieceColor::BLACK : PieceColor::WHITE,
                                   .type = is_dame ? PieceType::DAME : PieceType::PION});
    }
    return out;
}

// Static helper to construct a board from a Pieces map
Board Board::from_pieces(const Pieces& pieces) noexcept {
    Board b;
    for (const auto& [pos, info] : pieces) {
        const auto i = pos.hash();
        const auto m = static_cast<std::uint32_t>(1u) << static_cast<unsigned>(i);
        b.occ_bits_ |= m;
        if (info.color == PieceColor::BLACK) b.black_bits_ |= m;
        else b.black_bits_ &= ~m;
        if (info.type == PieceType::DAME) b.dame_bits_ |= m;
        else b.dame_bits_ &= ~m;
    }
    return b;
}
