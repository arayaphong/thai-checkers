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
#include <chrono>
#include <algorithm>
#include "Game.h"

class Traversal {
  public:
    Traversal() = default;
    // Run traversal for given wall-clock duration (default: 10s) and then stop, emitting a summary
    // via callback if set, otherwise printing to stdout.
    void traverse_for(std::chrono::milliseconds duration = std::chrono::seconds(10), const Game& game = Game());
    // Continue traversal preserving existing session state (for resume scenarios)
    void traverse_for_continue(std::chrono::milliseconds duration = std::chrono::seconds(10),
                               const Game& game = Game());

    // Cooperative stop retained for callbacks if needed in future extensions.
    void request_stop() noexcept { stop_.store(true, std::memory_order_relaxed); }

    // Event payloads and subscription APIs
    struct ResultEvent {
        std::size_t game_id;              // total games completed so far
        bool looping;                     // true if ended due to repetition
        std::optional<PieceColor> winner; // winner if not looping
        std::vector<std::size_t> history; // sequence of board state hashes (initial + each resulting state)
    };
    struct SummaryEvent {
        double wall_seconds;
        std::size_t games;          // current session games processed
        std::size_t previous_games; // games from previous sessions (checkpoint)
        std::size_t total_games;    // total cumulative games (final game ID)
        double throughput_games_per_sec;
        double cpu_seconds;      // -1 if unavailable
        double cpu_util_percent; // may exceed 100% on multi-core
        long rss_kb;             // -1 if unavailable
        long hwm_kb;             // -1 if unavailable
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
    void set_progress_interval(std::chrono::milliseconds interval) { progress_interval_ms_ = interval; }

    // Performance optimization controls
    void set_high_performance_mode(bool enabled) { high_performance_mode_ = enabled; }
    void set_memory_vs_speed_ratio(double ratio) { memory_speed_ratio_ = std::clamp(ratio, 0.0, 1.0); }
    void set_loop_detection_aggressive(bool enabled) { aggressive_loop_detection_ = enabled; }
    void set_task_depth_limit(int limit) { task_depth_limit_ = limit; }

    // Checkpoint / resume API
    bool save_checkpoint_compact(const std::string& path, bool compress = true) const; // legacy compatible public API
    void resume_or_start(const std::string& chk_path, Game root = Game());

  private:
    // Counter for completed games; updated concurrently
    std::atomic<std::size_t> game_count{1};
    std::size_t session_start_count{0};  // games count at session start (for tracking session progress)
    std::size_t global_session_total{0}; // cumulative total across all sessions

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
    // Minimal internal stack retained only for checkpoint compatibility (not exposed for manual stepping)
    struct Frame {
        Game game;
        std::uint32_t next_idx{0};
    };
    std::vector<Frame> work_stack_{}; // remains empty in timed traversal mode

    // Compact checkpoint load helper (internal)
    bool load_checkpoint_compact(const std::string& path);

    // Helpers for sharded loop database
    static constexpr std::size_t shard_for(std::size_t h) noexcept {
        // Fibonacci hashing, then take top k bits
        return (h * 11400714819323198485ull) >> (64 - k_shard_bits);
    }
    bool loop_seen(std::size_t h) const;
    void record_loop(std::size_t h);

    // Clear all loop-detection shards
    void clear_loops();

    // Performance optimization settings
    bool high_performance_mode_{false};
    double memory_speed_ratio_{0.5}; // 0.0 = max memory saving, 1.0 = max speed
    bool aggressive_loop_detection_{false};
    int task_depth_limit_{4};
};
