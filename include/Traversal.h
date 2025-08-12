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
#include "Game.h"

class Traversal {
  public:
    Traversal() = default;

    void traverse(const Game& game = Game());
    // Run traversal for given wall-clock duration (default: 10s) and then stop, emitting a summary
    // via callback if set, otherwise printing to stdout.
    void traverse_for(std::chrono::milliseconds duration = std::chrono::seconds(10), const Game& game = Game());

    // Controls
    void reset();                  // reset counters and internal loop DB, clear stop flag
    void request_stop() noexcept { // cooperative stop (safe to call from any thread)
        stop_.store(true, std::memory_order_relaxed);
    }
    [[nodiscard]] std::size_t games_completed() const noexcept {
        return game_count.load(std::memory_order_relaxed) - 1;
    }

    // Event payloads and subscription APIs
    struct ResultEvent {
        std::size_t game_id;                   // total games completed so far
        bool looping;                          // true if ended due to repetition
        std::optional<PieceColor> winner;      // winner if not looping
        std::vector<std::size_t> move_history; // sequence of chosen move indices (derived)
        std::vector<std::size_t> history;      // raw board_move_sequence (initial hash, then index, hash, ...)
    };
    struct SummaryEvent {
        double wall_seconds;
        std::size_t games;
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

    // Checkpoint / resume API
    struct Frame {
        Game game;
        std::uint32_t next_idx{0};
    };

    // Optimized checkpoint frame (stores only incremental state)
    struct CompactFrame {
        uint32_t board_occ, board_black, board_dame;
        uint8_t player;
        uint8_t looping;
        uint32_t next_idx;
        uint32_t parent_frame_idx; // Index into work_stack for parent
        uint32_t move_from_parent; // Move index that led to this state
        uint32_t current_hash;     // Board hash (for loop detection)
    };
    bool save_checkpoint(const std::string& path) const;
    bool save_checkpoint_compact(const std::string& path, bool compress = true) const;
    bool load_checkpoint(const std::string& path);
    bool load_checkpoint_compact(const std::string& path);
    void traverse_iterative(Game root); // single-thread iterative DFS (fills work_stack_)
    void resume_or_start(const std::string& chk_path, Game root = Game());
    void run_from_work_stack();      // continue using pre-populated work_stack_
    void start_root_only(Game root); // initialize work stack with root only (no exploration)
    bool step_one();                 // perform a single DFS expansion/result; returns false if finished
    std::size_t pending_frames() const noexcept { return work_stack_.size(); }

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

    // Iterative DFS state (only used for checkpoint-based traversal)
    std::vector<Frame> work_stack_{};
    void emit_result(const Game& game); // factored leaf handler for iterative path

    // Helpers for sharded loop database
    static constexpr std::size_t shard_for(std::size_t h) noexcept {
        // Fibonacci hashing, then take top k bits
        return (h * 11400714819323198485ull) >> (64 - k_shard_bits);
    }
    bool loop_seen(std::size_t h) const;
    void record_loop(std::size_t h);

    // Clear all loop-detection shards
    void clear_loops();

    // Serialize std::cout output to keep lines intact
    mutable std::mutex io_mutex;
};
