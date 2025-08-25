#include "utils.h"
#include <algorithm>
#include <fstream>
#include <format>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace {
constexpr long long k_milliseconds_per_second = 1000;
}

auto parse_timeout(std::string_view arg) -> std::optional<std::chrono::milliseconds> {
    if (arg.empty()) { return std::nullopt; }

    try {
        // Check for 'ms' suffix (milliseconds) FIRST - before checking 's'
        if (arg.ends_with("ms")) {
            const auto ms_str = arg.substr(0, arg.length() - 2);
            const long long milliseconds = std::stoll(std::string(ms_str));
            return std::chrono::milliseconds(milliseconds);
        }

        // Check for 's' suffix (seconds)
        if (arg.ends_with('s')) {
            const auto seconds_str = arg.substr(0, arg.length() - 1);
            const double seconds = std::stod(std::string(seconds_str));
            return std::chrono::milliseconds(static_cast<long long>(seconds * k_milliseconds_per_second));
        }

        // Default to seconds if no suffix
        const double seconds = std::stod(std::string(arg));
        return std::chrono::milliseconds(static_cast<long long>(seconds * k_milliseconds_per_second));
    } catch (const std::exception&) { return std::nullopt; }
}

void print_usage(const char* program_name) {
    std::cout << std::format("Usage: {} [--timeout DURATION]\n", program_name);
    std::cout << "Options:\n";
    std::cout << "  --timeout DURATION  Set timeout duration (e.g., 10s, 12.5s, 5000ms)\n";
    std::cout << "                      Default: 10s\n";
    std::cout << "  --help             Show this help message\n";
}

bool save_checkpoint_to_file(const std::vector<Traversal::CheckpointEntry>& cp, const std::string& filename) {
    std::ofstream ofs(filename, std::ios::trunc);
    if (!ofs) return false;

    for (std::size_t i = 0; i < cp.size(); ++i) {
        const auto& e = cp[i];
        ofs << e.index << '/';
        ofs << e.size;
        ofs << '\n';
    }

    return true;
}

std::string shift_decimal_right(const cpp_dec_float_50& num, std::size_t shift_amount) {
    if (shift_amount == 0) {
        std::ostringstream ss;
        ss << num;
        return ss.str();
    }

    // Format with high precision
    std::ostringstream ss;
    ss << std::setprecision(16) << num;
    std::string decimal_str = ss.str();

    // Handle scientific notation
    if (decimal_str.find('e') != std::string::npos || decimal_str.find('E') != std::string::npos) {
        std::ostringstream fixed_ss;
        fixed_ss << std::fixed << std::setprecision(16) << num;
        decimal_str = fixed_ss.str();
    }

    // Remove trailing zeros and ensure decimal point
    std::size_t dot_pos = decimal_str.find('.');
    if (dot_pos != std::string::npos) {
        std::size_t end = decimal_str.size();
        while (end > dot_pos + 1 && decimal_str[end - 1] == '0') { --end; }
        if (end == dot_pos + 1) {
            decimal_str.resize(dot_pos);
        } else {
            decimal_str.resize(end);
        }
    }

    if (decimal_str.find('.') == std::string::npos) { decimal_str += ".0"; }

    dot_pos = decimal_str.find('.');
    std::string integer_part = decimal_str.substr(0, dot_pos);
    std::string fractional_part = decimal_str.substr(dot_pos + 1);
    std::string all_digits = integer_part + fractional_part;

    // Create shifted result: "0." + zeros + all_digits
    std::string zeros(shift_amount, '0');
    std::string result = "0." + zeros + all_digits;

    // Remove trailing zeros but keep at least one decimal digit
    std::size_t last_non_zero = result.find_last_not_of('0');
    if (last_non_zero != std::string::npos) {
        if (result[last_non_zero] == '.') {
            result = result.substr(0, last_non_zero + 1) + '0';
        } else {
            result.erase(last_non_zero + 1);
        }
    }

    return result;
}

std::string add_big_decimals(const std::string& a, const std::string& b) {
    if (a == "0" || a == "0.0" || a.empty()) return b;
    if (b == "0" || b == "0.0" || b.empty()) return a;

    std::string a_str = a;
    std::string b_str = b;

    if (a_str.find('.') == std::string::npos) a_str += ".0";
    if (b_str.find('.') == std::string::npos) b_str += ".0";

    std::size_t a_dot = a_str.find('.');
    std::size_t b_dot = b_str.find('.');

    std::string a_int = a_str.substr(0, a_dot);
    std::string a_frac = a_str.substr(a_dot + 1);
    std::string b_int = b_str.substr(0, b_dot);
    std::string b_frac = b_str.substr(b_dot + 1);

    // Pad fractional parts
    std::size_t max_frac_len = std::max(a_frac.size(), b_frac.size());
    a_frac.append(max_frac_len - a_frac.size(), '0');
    b_frac.append(max_frac_len - b_frac.size(), '0');

    // Add fractional parts from right to left
    int carry = 0;
    std::string frac_result(max_frac_len, '0');
    for (std::size_t i = max_frac_len; i > 0; --i) {
        int digit_a = a_frac[i - 1] - '0';
        int digit_b = b_frac[i - 1] - '0';
        int sum = digit_a + digit_b + carry;
        frac_result[i - 1] = static_cast<char>('0' + (sum % 10));
        carry = sum / 10;
    }

    // Pad integer parts
    std::size_t max_int_len = std::max(a_int.size(), b_int.size());
    a_int.insert(a_int.begin(), max_int_len - a_int.size(), '0');
    b_int.insert(b_int.begin(), max_int_len - b_int.size(), '0');

    // Add integer parts from right to left
    std::string int_result(max_int_len, '0');
    for (std::size_t i = max_int_len; i > 0; --i) {
        int digit_a = a_int[i - 1] - '0';
        int digit_b = b_int[i - 1] - '0';
        int sum = digit_a + digit_b + carry;
        int_result[i - 1] = static_cast<char>('0' + (sum % 10));
        carry = sum / 10;
    }

    if (carry > 0) { int_result.insert(int_result.begin(), static_cast<char>('0' + carry)); }

    std::string result = int_result + '.' + frac_result;

    // Remove trailing zeros but keep one decimal digit
    std::size_t last_non_zero = result.find_last_not_of('0');
    if (last_non_zero != std::string::npos) {
        if (result[last_non_zero] == '.') {
            result = result.substr(0, last_non_zero + 1) + '0';
        } else {
            result.erase(last_non_zero + 1);
        }
    }

    return result;
}

int compare_big_decimals(const std::string& a, const std::string& b) {
    std::string a_str = a;
    std::string b_str = b;

    if (a_str.find('.') == std::string::npos) a_str += ".0";
    if (b_str.find('.') == std::string::npos) b_str += ".0";

    std::size_t a_dot = a_str.find('.');
    std::size_t b_dot = b_str.find('.');

    std::string a_int = a_str.substr(0, a_dot);
    std::string a_frac = a_str.substr(a_dot + 1);
    std::string b_int = b_str.substr(0, b_dot);
    std::string b_frac = b_str.substr(b_dot + 1);

    // Compare integer part lengths
    if (a_int.size() != b_int.size()) { return (a_int.size() > b_int.size()) ? 1 : -1; }
    if (a_int != b_int) { return (a_int > b_int) ? 1 : -1; }

    // Pad fractional parts and compare
    std::size_t max_frac_len = std::max(a_frac.size(), b_frac.size());
    a_frac.append(max_frac_len - a_frac.size(), '0');
    b_frac.append(max_frac_len - b_frac.size(), '0');

    if (a_frac == b_frac) { return 0; }
    return (a_frac > b_frac) ? 1 : -1;
}

std::string subtract_big_decimals(const std::string& a, const std::string& b) {
    int cmp = compare_big_decimals(a, b);
    if (cmp == 0) { return "0"; }

    // Determine which operand is larger
    std::string larger = (cmp > 0) ? a : b;
    std::string smaller = (cmp > 0) ? b : a;

    if (larger.find('.') == std::string::npos) larger += ".0";
    if (smaller.find('.') == std::string::npos) smaller += ".0";

    std::size_t l_dot = larger.find('.');
    std::size_t s_dot = smaller.find('.');

    std::string l_int = larger.substr(0, l_dot);
    std::string l_frac = larger.substr(l_dot + 1);
    std::string s_int = smaller.substr(0, s_dot);
    std::string s_frac = smaller.substr(s_dot + 1);

    // Pad fractional parts
    std::size_t max_frac_len = std::max(l_frac.size(), s_frac.size());
    l_frac.append(max_frac_len - l_frac.size(), '0');
    s_frac.append(max_frac_len - s_frac.size(), '0');

    // Pad integer parts
    std::size_t max_int_len = std::max(l_int.size(), s_int.size());
    l_int.insert(l_int.begin(), max_int_len - l_int.size(), '0');
    s_int.insert(s_int.begin(), max_int_len - s_int.size(), '0');

    // Subtract fractional parts from right to left
    int borrow = 0;
    std::string frac_result(max_frac_len, '0');
    for (std::size_t i = max_frac_len; i > 0; --i) {
        int da = (l_frac[i - 1] - '0') - borrow;
        int db = (s_frac[i - 1] - '0');
        if (da < db) {
            da += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        frac_result[i - 1] = static_cast<char>('0' + (da - db));
    }

    // Subtract integer parts from right to left
    std::string int_result(max_int_len, '0');
    for (std::size_t i = max_int_len; i > 0; --i) {
        int da = (l_int[i - 1] - '0') - borrow;
        int db = (s_int[i - 1] - '0');
        if (da < db) {
            da += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        int_result[i - 1] = static_cast<char>('0' + (da - db));
    }

    // Remove leading zeros from integer part
    std::size_t int_first_non_zero = int_result.find_first_not_of('0');
    if (int_first_non_zero != std::string::npos) {
        int_result.erase(0, int_first_non_zero);
    } else {
        int_result = "0";
    }

    // Combine integer and fractional parts
    std::string result = int_result + '.' + frac_result;

    // Trim trailing zeros while preserving one decimal digit
    std::size_t last_non_zero = result.find_last_not_of('0');
    if (last_non_zero != std::string::npos) {
        if (result[last_non_zero] == '.') {
            result = result.substr(0, last_non_zero + 1) + '0';
        } else {
            result.erase(last_non_zero + 1);
        }
    }

    return result;
}

std::string calculate_completion_percentage(const std::vector<Traversal::CheckpointEntry>& checkpoint) {
    if (checkpoint.empty()) { return "0.0"; }

    std::string final_sum = "0";

    // Process each checkpoint entry
    for (std::size_t idx = 0; idx < checkpoint.size(); ++idx) {
        const auto& entry = checkpoint[idx];

        // Calculate (index + 1 + 1) / size as a decimal
        // Following log processor: newNumerator = numerator + 1, where numerator = entry.index
        const auto numerator = entry.index;
        const auto new_numerator = numerator + 1; // This matches log processor's increment
        const auto denominator = entry.size;

        const cpp_dec_float_50 fraction =
            static_cast<cpp_dec_float_50>(new_numerator) / static_cast<cpp_dec_float_50>(denominator);

        // Shift decimal right by idx positions
        const std::string shifted = shift_decimal_right(fraction, idx);

        // Accumulate using string arithmetic
        final_sum = add_big_decimals(final_sum, shifted);
    }

    // Calculate theoretical value (10/9 as repeating decimal)
    // Determine required precision: at least 100 digits or length of final sum
    std::size_t precision = std::max<std::size_t>(100, final_sum.size());
    std::string theoretical = "1." + std::string(precision, '1');

    // Calculate difference = |theoretical - final_sum|
    std::string difference = subtract_big_decimals(theoretical, final_sum);

    // Calculate accuracy = 1 - difference
    std::string accuracy = subtract_big_decimals("1", difference);

    // Return the accuracy (this is the completion percentage we want)
    return accuracy;
}
