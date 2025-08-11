#include "Traversal.h"
#include <format>
#include <ranges>
#ifdef _OPENMP
#include <omp.h>
#endif

void Traversal::traverse(const Game& game) {
	const std::size_t move_count = game.move_count();

	// Fast-path check with shared lock
	{
		std::shared_lock read_lock(loop_db_mutex);
		if (looping_database.contains(game.board().hash())) {
			return;
		}
	}

	if (move_count == 0) {
		const auto& history = game.get_move_sequence();
		const std::size_t move_history_count = history.size() / 2;

		// Update loop database if needed
		if (game.is_looping()) {
			std::unique_lock write_lock(loop_db_mutex);
			looping_database.insert(game.board().hash());
		}

		// Print result atomically and assign a unique id
		{
			std::scoped_lock lock(io_mutex);
			const std::size_t id = game_count.fetch_add(1, std::memory_order_relaxed);
			std::cout << std::format("[{}] ", id);
			if (game.is_looping()) {
				std::cout << std::format("{} Moves ended in a loop!\n", move_history_count);
			} else {
				const char* winner = (game.player() == PieceColor::BLACK ? "White" : "Black");
				std::cout << std::format("{} Moves ended with a {} win!\n", move_history_count, winner);
			}
		}
		return;
	}

	// Spawn child traversals; prefer OpenMP tasks if available
#ifdef _OPENMP
	if (omp_in_parallel()) {
		for (std::size_t i = 0; i < move_count; ++i) {
			const Game base = game; // snapshot for this child
			#pragma omp task firstprivate(base, i)
			{
				Game next_game = base; // copy per task
				next_game.select_move(i);
				traverse(next_game); // may create nested tasks
			}
		}
	} else {
		// Create a single parallel region at the root
		#pragma omp parallel
		{
			#pragma omp single nowait
			{
				for (std::size_t i = 0; i < move_count; ++i) {
					const Game base = game; // snapshot for this child
					#pragma omp task firstprivate(base, i)
					{
						Game next_game = base;
						next_game.select_move(i);
						traverse(next_game);
					}
				}
			}
		}
	}
#else
	for (std::size_t i = 0; i < move_count; ++i) {
		Game next_game = game; // use copy constructor
		next_game.select_move(i);
		traverse(next_game);
	}
#endif
}
