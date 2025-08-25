#pragma once

#include "Traversal.h"
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using boost::multiprecision::cpp_dec_float_50;

// Timeout parsing utility
auto parse_timeout(std::string_view arg) -> std::optional<std::chrono::milliseconds>;

// Usage information
void print_usage(const char* program_name);

// Checkpoint file operations
bool save_checkpoint_to_file(const std::vector<Traversal::CheckpointEntry>& cp, const std::string& filename);

// High-precision decimal arithmetic utilities
std::string shift_decimal_right(const cpp_dec_float_50& num, std::size_t shift_amount);
std::string add_big_decimals(const std::string& a, const std::string& b);
int compare_big_decimals(const std::string& a, const std::string& b);
std::string subtract_big_decimals(const std::string& a, const std::string& b);

// Completion percentage calculation using log processor algorithm
std::string calculate_completion_percentage(const std::vector<Traversal::CheckpointEntry>& checkpoint);
