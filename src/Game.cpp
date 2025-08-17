#include "Game.h"
#include "Board.h"
#include "Explorer.h"
#include <algorithm>
#include <cstddef>
#include "Piece.h"
#include <cstdint>
#include <iostream>
#include <format>
#include <ranges>
#include <string>
#include <utility>
#include <unordered_map>

auto piece_symbol(bool is_black, bool is_dame) -> std::string {
    return is_black ? (is_dame ? "□" : "○") : (is_dame ? "■" : "●");
}

auto board_to_string(const Board& board) -> std::string {
    return [&]() {
        std::string str = "   ";
        for (char col : std::views::iota('A', 'I')) { str += std::format("{} ", col); }
        str += "\n";

        for (int row_index : std::views::iota(0, 8)) {
            str += std::format(" {} ", row_index + 1);
            for (int col_index : std::views::iota(0, 8)) {
                const bool is_light_square = (row_index + col_index) % 2 == 0;

                const std::string symbol = [&]() -> std::string {
                    if (is_light_square) {
                        return ".";
                    }                         // Only create Position for valid black squares
                        const auto pos = Position{col_index, row_index};
                        if (board.is_occupied(pos)) {
                            return piece_symbol(board.is_black_piece(pos), board.is_dame_piece(pos));
                        }                             return " ";
                       
                   
                }();

                str += std::format("{} ", symbol);
            }
            str += "\n";
        }
        return str;
    }();
}

auto Game::get_choices() const -> const std::vector<Move>& {
    if (!choices_dirty_) { return choices_cache_;
}

    choices_cache_.clear();

    const auto moveable_pieces = get_moveable_pieces();

    // Gather and sort positions for deterministic iteration order
    std::vector<Position> from_positions;
    from_positions.reserve(moveable_pieces.size());
    for (const auto& piece_pair : moveable_pieces) { from_positions.push_back(piece_pair.first);
}
    std::sort(from_positions.begin(), from_positions.end(),
              [](const Position& pos_a, const Position& pos_b) { return pos_a.hash() < pos_b.hash(); });

    bool any_capture = false;
    for (const auto& from : from_positions) {
        const auto& legals = moveable_pieces.at(from);
        struct TmpMove {
            Position to;
            std::vector<Position> captured;
        };
        std::vector<TmpMove> tmp;
        tmp.reserve(legals.size());
        for (std::size_t i = 0; i < legals.size(); ++i) {
            const bool is_cap = legals.has_captured();
            std::vector<Position> captured = is_cap ? legals.get_capture_pieces(i) : std::vector<Position>{};
            any_capture = any_capture || !captured.empty();
            tmp.push_back(TmpMove{legals.get_position(i), std::move(captured)});
        }
        // Sort: captures first handled later, but ensure deterministic order by (to, captured...)
        std::sort(tmp.begin(), tmp.end(), [](const TmpMove& move_a, const TmpMove& move_b) {
            if (move_a.to.hash() != move_b.to.hash()) { return move_a.to.hash() < move_b.to.hash();
}
            return move_a.captured < move_b.captured; // lexicographic on positions
        });
        for (auto& move_item : tmp) { choices_cache_.push_back(Move{from, move_item.to, std::move(move_item.captured)}); }
    }

    if (any_capture) {
        // Keep only capture moves
        std::vector<Move> capture_only;
        capture_only.reserve(choices_cache_.size());
        for (auto& move_item : choices_cache_) {
            if (move_item.is_capture()) { capture_only.push_back(std::move(move_item));
}
}
        choices_cache_.swap(capture_only);
    }

    choices_dirty_ = false;
    return choices_cache_;
}

void Game::push_history_state() {
    // Count the current board state with the current player
    const auto key = get_position_key(current_board, player());
    position_count[key]++;
}

auto Game::get_position_key(const Board& board, PieceColor player) noexcept -> std::size_t {
    // Combine board hash with player using bit manipulation
    // Use the board hash and set/clear the highest bit based on player color
    std::size_t hash = board.hash();
    constexpr std::size_t kPlayerBit = std::size_t(1) << (sizeof(std::size_t) * 8 - 1);
    if (player == PieceColor::BLACK) {
        hash |= kPlayerBit; // Set highest bit for BLACK
    } else {
        hash &= ~kPlayerBit; // Clear highest bit for WHITE
    }
    return hash;
}

auto Game::get_moveable_pieces() const -> std::unordered_map<Position, Legals> {
    const auto explorer = Explorer(current_board);
    const auto pieces = current_board.get_pieces(player());

    std::unordered_map<Position, Legals> out;
    out.reserve(pieces.size());
    for (const auto& [pos, _info] : pieces) {
        auto opts = explorer.find_valid_moves(pos);
        if (!opts.empty()) { out.emplace(pos, std::move(opts)); }
    }
    return out;
}

void Game::execute_move(const Move& move) {
    const auto& from_pos = move.from;
    const auto& to_pos = move.to;
    const auto& captured = move.captured;

    // Execute the specified move
    auto new_board = Board::copy(current_board);
    new_board.move_piece(from_pos, to_pos);

    // Check if the piece is promoted to a dame
    if (to_pos.y() == 0 || to_pos.y() == Position::board_size - 1) { new_board.promote_piece(to_pos); }

    // Remove captured pieces
    for (const auto& pos : captured) { new_board.remove_piece(pos); }

    // Update current player and board state
    current_board = new_board;

    // Store the new board state in history
    board_history.push_back(current_board);

    // Push history state
    push_history_state();

    // Check for repeated board states
    is_looping_ = seen(current_board);

    // Mark cached choices dirty due to state change
    choices_dirty_ = true;
}

auto Game::seen(const Board& board) const noexcept -> bool {
    const auto key = get_position_key(board, player());
    const auto position_iterator = position_count.find(key);
    // 3-fold repetition rule: position with same player becomes a draw if it occurs 3 times
    constexpr int kRepetitionLimit = 3;
    return position_iterator != position_count.end() && position_iterator->second >= kRepetitionLimit;
}

void Game::undo_move() {
    if (index_history.empty()) {
        std::cerr << "No moves to undo.\n";
        return;
    }

    // Remove the current board state from history, but keep at least the initial state
    if (board_history.size() > 1) {
        // Decrease the count for the current position with current player
        const auto key = get_position_key(current_board, player());
        auto position_iterator = position_count.find(key);
        if (position_iterator != position_count.end()) {
            position_iterator->second--;
            if (position_iterator->second == 0) { position_count.erase(position_iterator); }
        }

        board_history.pop_back();
        // Restore the previous board state
        current_board = board_history.back();

        // Pop the last move index from history
        index_history.pop_back();

        is_looping_ = false;   // Reset loop detection
        choices_dirty_ = true; // Mark choices as dirty to recompute
    } else {
        std::cerr << "Cannot undo: already at initial state.\n";
    }
}

void Game::select_move(std::size_t index) {
    const auto& choices = get_choices();

    // Store the selected move index in history
    index_history.push_back(static_cast<uint8_t>(index));

    execute_move(choices[index]);
}

void Game::print_board() const noexcept { std::cout << board_to_string(current_board); }

void Game::print_choices() const {
    const auto& choices = get_choices();
    for (const auto& move_choice : choices) {
        std::cout << "From: " << move_choice.from.to_string() << " To: " << move_choice.to.to_string();
        if (!move_choice.captured.empty()) {
            std::cout << " (Captures: ";
            for (std::size_t i = 0; i < move_choice.captured.size(); ++i) {
                std::cout << move_choice.captured[i].to_string();
                if (i < move_choice.captured.size() - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << ")";
        }
        std::cout << std::endl;
    }
}

auto Game::player() const noexcept -> PieceColor {
    // Player alternates based on number of moves made
    // Even number of moves (0, 2, 4, ...) = White's turn
    // Odd number of moves (1, 3, 5, ...) = Black's turn
    if (index_history.size() % 2 == 0) {
        return PieceColor::WHITE;
    }         return PieceColor::BLACK;
   
}
