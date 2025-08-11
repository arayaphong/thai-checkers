#pragma once

#include <iostream>
#include <cstddef>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include "Game.h"

class Traversal {
public:
    Traversal() = default;

    void traverse(const Game& game = Game());

private:
    // Counter for completed games; updated concurrently
    std::atomic<std::size_t> game_count{1};

    // Hashes of boards known to be looping; guarded by shared mutex
    std::unordered_set<std::size_t> looping_database;
    mutable std::shared_mutex loop_db_mutex;

    // Serialize std::cout output to keep lines intact
    mutable std::mutex io_mutex;
};
