#include "Game.h"
#include "Explorer.h"
#include <iostream>
#include <format>
#include <ranges>

std::string piece_symbol(bool is_black, bool is_dame) {
    return is_black ? (is_dame ? "□" : "○") : (is_dame ? "■" : "●");
}

std::string board_to_string(const Board& board) {
    return [&]() {
        std::string str = "   ";
        for (char col : std::views::iota('A', 'I')) { str += std::format("{} ", col); }
        str += "\n";

        for (auto i : std::views::iota(0, 8)) {
            str += std::format(" {} ", i + 1);
            for (auto j : std::views::iota(0, 8)) {
                const bool is_light_square = (i + j) % 2 == 0;

                const std::string symbol = [&]() -> std::string {
                    if (is_light_square) {
                        return ".";
                    } else {
                        // Only create Position for valid black squares
                        const auto pos = Position{j, i};
                        if (board.is_occupied(pos)) {
                            return piece_symbol(board.is_black_piece(pos), board.is_dame_piece(pos));
                        } else {
                            return " ";
                        }
                    }
                }();

                str += std::format("{} ", symbol);
            }
            str += "\n";
        }
        return str;
    }();
}

Game::Game() : current_player(PieceColor::WHITE), current_board(Board::setup()) {
    board_move_sequence.push_back(current_board);
    // Track initial state for repetition detection
    seen_states_.clear();
    seen_states_.insert(state_key());
    game_over_ = false;
}

const std::vector<Move>& Game::get_choices() const {
    if (!choices_dirty_) return choices_cache_;

    choices_cache_.clear();

    if (game_over_) {
        choices_dirty_ = false;
        return choices_cache_;
    }
    const auto moveable_pieces = get_moveable_pieces();

    // Gather and sort positions for deterministic iteration order
    std::vector<Position> from_positions;
    from_positions.reserve(moveable_pieces.size());
    for (const auto& kv : moveable_pieces) from_positions.push_back(kv.first);
    std::sort(from_positions.begin(), from_positions.end(),
              [](const Position& a, const Position& b) { return a.hash() < b.hash(); });

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
        std::sort(tmp.begin(), tmp.end(), [](const TmpMove& a, const TmpMove& b) {
            if (a.to.hash() != b.to.hash()) return a.to.hash() < b.to.hash();
            return a.captured < b.captured; // lexicographic on positions
        });
        for (auto& m : tmp) { choices_cache_.push_back(Move{from, m.to, std::move(m.captured)}); }
    }

    if (any_capture) {
        // Keep only capture moves
        std::vector<Move> capture_only;
        capture_only.reserve(choices_cache_.size());
        for (auto& m : choices_cache_)
            if (m.is_capture()) capture_only.push_back(std::move(m));
        choices_cache_.swap(capture_only);
    }

    choices_dirty_ = false;
    return choices_cache_;
}

Game Game::copy(const Game& other) {
    // Delegate to the compiler-generated copy constructor to keep this
    // function correct and future-proof if members are added/changed.
    return other;
}

std::unordered_map<Position, Legals> Game::get_moveable_pieces() const {
    // IMPORTANT: Don't build a ranges view over a temporary returned from get_pieces().
    // That creates a dangling view once the temporary is destroyed, leading to UB/hangs.
    const auto analyzer = Explorer(current_board);
    const auto pieces = current_board.get_pieces(current_player); // materialize first

    std::unordered_map<Position, Legals> out;
    out.reserve(pieces.size());
    for (const auto& [pos, _info] : pieces) {
        auto opts = analyzer.find_valid_moves(pos);
        if (!opts.empty()) { out.emplace(pos, std::move(opts)); }
    }
    return out;
}

Board Game::execute_move(const Move& move) {
    const auto& from = move.from;
    const auto& to = move.to;
    const auto& captured = move.captured;

    // Hard removal: assume move is valid (built from get_choices()).

    // Execute the specified move
    auto new_board = Board::copy(current_board);
    new_board.move_piece(from, to);

    // Check if the piece is promoted to a dame
    if (to.y() == 0 || to.y() == Position::board_size - 1) { new_board.promote_piece(to); }

    // Remove captured pieces
    for (const auto& pos : captured) { new_board.remove_piece(pos); }

    // Keep track of the new board state
    board_move_sequence.push_back(new_board);

    // Update current player and board state
    current_board = new_board;
    current_player = (current_player == PieceColor::WHITE) ? PieceColor::BLACK : PieceColor::WHITE;

    // Mark cached choices dirty due to state change
    choices_dirty_ = true;

    // Repetition detection: if this state was seen, end the game (treat as draw)
    const auto key = state_key();
    if (!seen_states_.insert(key).second) { game_over_ = true; }

    return new_board;
}

std::size_t Game::move_count() const {
    if (game_over_) return 0;
    // Safe to call get_choices(); if it were to throw, we wouldn't mark noexcept.
    // Here we assume underlying operations won't throw in typical use.
    return get_choices().size();
}

void Game::select_move(std::size_t index) {
    const auto& choices = get_choices();

    // Keep track of the move sequence
    board_move_sequence.push_back(index);

    // Execute the selected move
    (void)execute_move(choices[index]);
}

void Game::print_board() const noexcept { std::cout << board_to_string(current_board); }

void Game::print_choices() const {
    const auto& choices = get_choices();
    for (const auto& m : choices) {
        std::cout << "From: " << m.from.to_string() << " To: " << m.to.to_string();
        if (!m.captured.empty()) {
            std::cout << " (Captures: ";
            for (std::size_t i = 0; i < m.captured.size(); ++i) {
                std::cout << m.captured[i].to_string();
                if (i + 1 < m.captured.size()) std::cout << ", ";
            }
            std::cout << ")";
        }
        std::cout << '\n';
    }
}
