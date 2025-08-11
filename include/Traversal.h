#pragma once

#include <iostream>
#include <cstddef>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <array>
#include <functional>
#include <optional>
#include <vector>
#include "Game.h"

class Traversal {
public:
    Traversal() = default;

    void traverse(const Game& game = Game());
    // Run traversal for given wall-clock duration and then stop, printing a summary.
    void traverse_for(std::chrono::milliseconds duration, const Game& game = Game());

    // Event payloads and subscription APIs
    struct ResultEvent {
        std::size_t game_id;       // total games completed so far
        bool looping;                  // true if ended due to repetition
        std::optional<PieceColor> winner; // winner if not looping
    std::vector<std::size_t> move_history; // sequence of chosen move indices (derived)
    std::vector<std::size_t> history;      // raw board_move_sequence (initial hash, then index, hash, ...)
    };
    struct SummaryEvent {
        double wall_seconds;
        std::size_t games;
        double throughput_games_per_sec;
        double cpu_seconds;            // -1 if unavailable
        double cpu_util_percent;       // may exceed 100% on multi-core
        long rss_kb;                   // -1 if unavailable
        long hwm_kb;                   // -1 if unavailable
    };
    struct ProgressEvent {
        std::size_t games;
    };
    void set_result_callback(std::function<void(const ResultEvent&)> cb) {
        std::scoped_lock lock(cb_mutex_);
        result_cb_ = std::move(cb);
    }
    void set_summary_callback(std::function<void(const SummaryEvent&)> cb) {
        std::scoped_lock lock(cb_mutex_);
        summary_cb_ = std::move(cb);
    }
    void set_progress_callback(std::function<void(const ProgressEvent&)> cb) {
        std::scoped_lock lock(cb_mutex_);
        progress_cb_ = std::move(cb);
    }
    void set_progress_interval(std::chrono::milliseconds interval) {
        progress_interval_ms_ = interval;
    }

private:
    // Counter for completed games; updated concurrently
    std::atomic<std::size_t> game_count{1};

    // Sharded hash sets for loop detection to reduce lock contention under parallel load
    static constexpr std::size_t k_shard_bits = 6; // 64 shards
    static constexpr std::size_t k_shard_count = (1u << k_shard_bits);
    struct LoopShard {
        std::unordered_set<std::size_t> set;
        mutable std::shared_mutex mtx;
    };
    std::array<LoopShard, k_shard_count> loop_shards_{};

    // Depth-aware traversal to limit task creation overhead
    void traverse_impl(const Game& game, int depth);

    // Timing and stop coordination
    std::atomic<bool> stop_{false};
    std::chrono::steady_clock::time_point start_tp_{};
    std::chrono::steady_clock::time_point deadline_tp_{};
    std::chrono::milliseconds progress_interval_ms_{std::chrono::milliseconds(3000)};

    // Summary helpers
    void print_summary() const;
    static double process_cpu_seconds();
    static long proc_status_kb(const char* key); // reads /proc/self/status

    // Subscribers
    std::function<void(const ResultEvent&)> result_cb_;
    std::function<void(const SummaryEvent&)> summary_cb_;
    std::function<void(const ProgressEvent&)> progress_cb_;
    mutable std::mutex cb_mutex_;

    // Helpers for sharded loop database
    static constexpr std::size_t shard_for(std::size_t h) noexcept {
        // Fibonacci hashing, then take top k bits
        return (h * 11400714819323198485ull) >> (64 - k_shard_bits);
    }
    bool loop_seen(std::size_t h) const;
    void record_loop(std::size_t h);

    // Serialize std::cout output to keep lines intact
    mutable std::mutex io_mutex;
};
