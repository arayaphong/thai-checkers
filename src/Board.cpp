#include "Board.h"
#include "Piece.h"
#include "Position.h"
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <utility>

// NOLINTNEXTLINE(bugprone-exception-escape)
auto Board::setup_pieces() -> Board {
    Board board;
    // Black rows
    for (int row = 0; row < BoardConstants::kInitialPieceRows; ++row) {
        const int start_col = (row % 2 == 0) ? 1 : 0;
        for (int i = 0; i < BoardConstants::kPiecesPerRow; ++i) {
            const int col = start_col + i * 2;
            const auto position = Position::from_coords(col, row);
            const auto index = static_cast<unsigned>(position.hash());
            const auto mask = static_cast<std::uint32_t>(1U) << index;
            board.occ_bits_ |= mask;
            board.black_bits_ |= mask;
            board.dame_bits_ &= ~mask;
        }
    }
    // White rows
    for (int row = BoardConstants::kWhiteStartingRow; row < BoardConstants::kBoardDimension; ++row) {
        const int start_col = (row % 2 == 0) ? 1 : 0;
        for (int i = 0; i < BoardConstants::kPiecesPerRow; ++i) {
            const int col = start_col + i * 2;
            const auto position = Position::from_coords(col, row);
            const auto index = static_cast<unsigned>(position.hash());
            const auto mask = static_cast<std::uint32_t>(1U) << index;
            board.occ_bits_ |= mask;
            board.black_bits_ &= ~mask;
            board.dame_bits_ &= ~mask;
        }
    }
    return board;
}

auto Board::setup() -> Board { return setup_pieces(); }

auto Board::from_hash(std::size_t hash) -> Board {
    Board board;

    std::bitset<BoardConstants::kBitsetSize> bits(hash);
    // Occupancy in bits 32..63
    board.occ_bits_ = static_cast<std::uint32_t>(hash >> BoardConstants::kBoardSquares);

    // Reconstruct dame_bits_ and black_bits_ from first 16 occupied squares' metadata
    board.dame_bits_ = 0;
    board.black_bits_ = 0;
    int count = 0;
    for (int i = 0; i < BoardConstants::kBoardSquares && count < BoardConstants::kMaxPiecesPerSide; ++i) {
        const auto mask = static_cast<std::uint32_t>(1U) << i;
        if ((board.occ_bits_ & mask) == 0U) {
            continue;
        }

        // Dame info is in bits[count], black info is in bits[count + 16]
        if (bits[count]) {
            board.dame_bits_ |= mask;
        }
        if (bits[count + BoardConstants::kMaxPiecesPerSide]) {
            board.black_bits_ |= mask;
        }
        ++count;
    }

    return board;
}

auto Board::hash() const noexcept -> std::size_t {
    // Compose a 64-bit hash: top 32 bits are occupancy layout, lower bits summarize first up-to-16 pieces' color/type.
    std::bitset<BoardConstants::kBitsetSize> bits;
    // Occupancy in bits 32..63
    for (int i = 0; i < BoardConstants::kBoardSquares; ++i) {
        bits[i + BoardConstants::kBoardSquares] = ((occ_bits_ >> i) & 1U) != 0U;
    }
    // Encode first 16 occupied squares' metadata (type in 0..15, color in 16..31)
    int count = 0;
    for (int i = 0; i < BoardConstants::kBoardSquares && count < BoardConstants::kMaxPiecesPerSide; ++i) {
        const auto mask = static_cast<std::uint32_t>(1U) << i;
        if ((occ_bits_ & mask) == 0U) {
            continue;
        }
        bits[count] = ((dame_bits_ & mask) != 0U);
        bits[count + BoardConstants::kMaxPiecesPerSide] = ((black_bits_ & mask) != 0U);
        ++count;
    }
    return bits.to_ullong();
}

auto Board::is_occupied(const Position& pos) const noexcept -> bool {
    if (!is_valid_position(pos)) {
        return false;
    }
    const auto idx = static_cast<unsigned>(pos.hash());
    return (occ_bits_ & (static_cast<std::uint32_t>(1U) << idx)) != 0U;
}

auto Board::is_black_piece(const Position& pos) const noexcept -> bool {
    const auto idx = static_cast<unsigned>(pos.hash());
    const auto mask = static_cast<std::uint32_t>(1U) << idx;
    return (black_bits_ & mask) != 0U;
}

auto Board::is_dame_piece(const Position& pos) const noexcept -> bool {
    const auto idx = static_cast<unsigned>(pos.hash());
    const auto mask = static_cast<std::uint32_t>(1U) << idx;
    return (dame_bits_ & mask) != 0U;
}

void Board::promote_piece(const Position& pos) noexcept {
    if (!is_valid_position(pos)) {
        return;
    }
    const auto idx = static_cast<unsigned>(pos.hash());
    const auto mask = static_cast<std::uint32_t>(1U) << idx;
    if ((occ_bits_ & mask) == 0U) {
        return;
    }
    dame_bits_ |= mask;
}

void Board::move_piece(const Position& from_pos, const Position& to_pos) noexcept {
    const auto from_idx = static_cast<unsigned>(from_pos.hash());
    const auto to_idx = static_cast<unsigned>(to_pos.hash());
    const auto from_mask = static_cast<std::uint32_t>(1U) << from_idx;
    const auto to_mask = static_cast<std::uint32_t>(1U) << to_idx;
    // Only move if from occupied and to empty
    if ((occ_bits_ & from_mask) == 0U) {
        return;
    }
    if ((occ_bits_ & to_mask) != 0U) {
        return;
    }
    const bool was_black = (black_bits_ & from_mask) != 0U;
    const bool was_dame = (dame_bits_ & from_mask) != 0U;
    // Clear from
    occ_bits_ &= ~from_mask;
    black_bits_ &= ~from_mask;
    dame_bits_ &= ~from_mask;
    // Set to
    occ_bits_ |= to_mask;
    if (was_black) {
        black_bits_ |= to_mask;
    } else {
        black_bits_ &= ~to_mask;
    }
    if (was_dame) {
        dame_bits_ |= to_mask;
    } else {
        dame_bits_ &= ~to_mask;
    }
}

void Board::remove_piece(const Position& pos) noexcept {
    if (!is_valid_position(pos)) {
        return;
    }
    const auto idx = static_cast<unsigned>(pos.hash());
    const auto mask = static_cast<std::uint32_t>(1U) << idx;
    occ_bits_ &= ~mask;
    black_bits_ &= ~mask;
    dame_bits_ &= ~mask;
}

auto Board::get_pieces(PieceColor color) const -> Pieces {
    Pieces out;
    out.reserve(BoardConstants::kPiecesReserveSize);
    for (int i = 0; i < BoardConstants::kBoardSquares; ++i) {
        const auto mask = static_cast<std::uint32_t>(1U) << i;
        if ((occ_bits_ & mask) == 0U) {
            continue;
        }
        const bool is_black = (black_bits_ & mask) != 0U;
        const bool is_dame = (dame_bits_ & mask) != 0U;
        if ((color == PieceColor::BLACK) != is_black) {
            continue;
        }
        const auto pos = Position::from_index(i);
        out.emplace(pos, PieceInfo{.color = is_black ? PieceColor::BLACK : PieceColor::WHITE,
                                   .type = is_dame ? PieceType::DAME : PieceType::PION});
    }
    return out;
}

// Static helper to construct a board from a Pieces map
auto Board::from_pieces(const Pieces& pieces) -> Board {
    Board board;
    for (const auto& [pos, info] : pieces) {
        const auto index = pos.hash();
        const auto mask = static_cast<std::uint32_t>(1U) << static_cast<unsigned>(index);
        board.occ_bits_ |= mask;
        if (info.color == PieceColor::BLACK) {
            board.black_bits_ |= mask;
        } else {
            board.black_bits_ &= ~mask;
        }
        if (info.type == PieceType::DAME) {
            board.dame_bits_ |= mask;
        } else {
            board.dame_bits_ &= ~mask;
        }
    }
    return board;
}