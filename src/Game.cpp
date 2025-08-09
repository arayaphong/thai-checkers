#include "Game.h"
#include "Explorer.h"
#include <iostream>
#include <format>
#include <ranges>

std::string piece_symbol(bool is_black, bool is_dame) {
    return is_black
               ? (is_dame ? "□" : "○")
               : (is_dame ? "■" : "●");
}

std::string board_to_string(const Board& board) {
    return [&]() {
        std::string str = "   ";
        for (char col : std::views::iota('A', 'I')) {
            str += std::format("{} ", col);
        }
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
    model.insert(current_board);
}

std::vector<Move> Game::get_choices() const {
    std::vector<Move> choices;
    const auto moveable_pieces = get_moveable_pieces();

    bool any_capture = false;
    for (const auto& [from, options] : moveable_pieces) {
        for (std::size_t i = 0; i < options.size(); ++i) {
            const bool is_cap = options.has_captured();
            std::vector<Position> captured = is_cap ? options.get_capture_pieces(i) : std::vector<Position>{};
            any_capture = any_capture || !captured.empty();
            choices.push_back(Move{from, options.get_position(i), std::move(captured)});
        }
    }

    if (any_capture) {
        // Keep only capture moves
        std::vector<Move> capture_only;
        capture_only.reserve(choices.size());
        for (auto& m : choices) if (m.is_capture()) capture_only.push_back(std::move(m));
        choices.swap(capture_only);
    }
    return choices;
}

std::unordered_map<Position, Options> Game::get_moveable_pieces() const {
    const auto analyzer = Explorer(current_board);
    
    auto valid_moves_view = current_board.get_pieces(current_player)
        | std::views::transform([&analyzer](const auto& pair) {
            return std::make_pair(pair.first, analyzer.find_valid_moves(pair.first));
        })
        | std::views::filter([](const auto& pair) { 
            return !pair.second.empty(); 
        });

    return std::unordered_map<Position, Options>(valid_moves_view.begin(), valid_moves_view.end());
}

bool Game::is_legal_move(const Move& move) const {
    const auto legal = get_choices();
    return std::ranges::any_of(legal, [&move](const Move& m){ return m == move; });
}

Board Game::execute_move(const Move& move) {
    const auto& from = move.from;
    const auto& to = move.to;
    const auto& captured = move.captured;

    if (!current_board.is_occupied(from)) [[unlikely]] {
        throw std::invalid_argument("No piece at the specified position");
    }

    if (!is_legal_move(move)) [[unlikely]] {
        throw std::invalid_argument("Attempted to execute an illegal move");
    }

    // Execute the specified move
    auto new_board = Board::copy(current_board);
    new_board.move_piece(from, to);

    // Check if the piece is promoted to a dame
    if (to.y() == 0 || to.y() == Position::board_size - 1) {
        new_board.promote_piece(to);
    }

    // Remove captured pieces
    for (const auto& pos : captured) {
        if (new_board.is_occupied(pos)) {
            new_board.remove_piece(pos);
        }
    }

    // Keep track of the new board state
    model.insert(new_board);

    // Update current player and board state
    current_board = new_board;
    current_player = (current_player == PieceColor::WHITE) ? PieceColor::BLACK : PieceColor::WHITE;

    return new_board;
}

void Game::print_board() const {
    std::cout << board_to_string(current_board);
}
