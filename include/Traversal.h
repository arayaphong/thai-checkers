#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

class Game;

class Traversal {
public:
    struct Options {
        int split_depth = 5;                       // depth to split the tree into a frontier
        std::uint64_t start_offset = 0;            // number of frontier seeds to skip (resume cursor)
        std::uint64_t max_frontier = 0;            // 0 = process all remaining frontier seeds
        std::string seed_token;                    // Alternate encoding of resume cursor: "D:OFFSET"
        bool verbose = false;                      // print progress
        int max_depth = 200;                       // safety cap for DFS to prevent cycles/infinite games
        std::chrono::milliseconds time_limit{0};   // 0 = no time limit
        std::uint64_t node_limit = 0;              // 0 = unlimited nodes
        std::uint64_t leaf_limit = 0;              // 0 = unlimited leaves
    };

    struct Result {
        std::uint64_t total_frontier = 0;          // total frontier size at split_depth (for reference)
        std::uint64_t processed_seeds = 0;         // how many frontier seeds processed in this run
        std::uint64_t next_offset = 0;             // cursor to resume from next time
        std::uint64_t nodes_visited = 0;           // internal nodes expanded in post-frontier DFS
        std::uint64_t terminal_positions = 0;      // number of terminal leaves reached (complete games)
        int max_depth_reached = 0;                 // maximum depth from root reached during this run
        std::string resume_token;                  // formatted as "D:OFFSET"
    };

    static Result run(const Options& opts);

private:
    static void apply_seed_token(const std::string &token, int &split_depth, std::uint64_t &start_offset);
    static bool replay_path(Game &g, const std::vector<std::size_t> &path, int &max_depth_reached);
    static void enumerate_frontier(const Game &base,
                                   int split_depth,
                                   std::uint64_t skip,
                                   std::uint64_t limit,
                                   std::vector<std::vector<std::size_t>> &out,
                                   std::uint64_t &total_frontier_count);
    static void dfs_to_terminal(Game &g,
                                std::uint64_t &nodes,
                                std::uint64_t &leaves,
                                int current_depth,
                                int &max_depth_reached,
                                int max_depth_limit,
                                const std::chrono::steady_clock::time_point *deadline,
                                std::atomic<bool> *stop,
                                const std::uint64_t node_cap,
                                const std::uint64_t leaf_cap);
};
