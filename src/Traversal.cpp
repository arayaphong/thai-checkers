#include "Traversal.h"
#include <format>
#include <ranges>

void Traversal::traverse(const Game& game) {
	const std::size_t move_count = game.move_count();
	if (looping_database.contains(game.board().hash())) {
		return;
	}

	if (move_count == 0) {
		std::cout << std::format("[{}] ", game_count);
		const auto& history = game.get_move_sequence();
		const std::size_t move_history_count = history.size() / 2;
		if (game.is_looping()) {
			looping_database.insert(game.board().hash());
			std::cout << std::format("{} Moves ended in a loop!\n", move_history_count);
		} else {
			const char* winner = (game.player() == PieceColor::BLACK ? "White" : "Black");
			std::cout << std::format("{} Moves ended with a {} win!\n", move_history_count, winner);
		}
		++game_count;
		return;
	}

	for (std::size_t i = 0; i < move_count; ++i) {
		Game next_game = game; // use copy constructor
		next_game.select_move(i);
		traverse(next_game);
	}
}
