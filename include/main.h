#pragma once
void test_da_capture_sequences();

void test_da_valid_moves();

void test_pa_valid_moves();

void test_pa_capture_sequences();

void print_choices(const std::vector<std::pair<Position, std::size_t>> &choices1, Game &game);

void random_game_play();
