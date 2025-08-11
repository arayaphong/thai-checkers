#pragma once

#include <iostream>
#include <cstddef>
#include <unordered_set>
#include "Game.h"

class Traversal {
public:
    Traversal() = default;

    void traverse(const Game& game = Game());

private:
    std::size_t game_count = 1;
    std::unordered_set<std::size_t> looping_database;
};
