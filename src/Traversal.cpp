#include "Traversal.h"
#include "Game.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#ifdef _OPENMP
#include <omp.h>
#endif

void Traversal::apply_seed_token(const std::string &token, int &split_depth, std::uint64_t &start_offset)
{
    if (token.empty())
        return;
    const auto pos = token.find(':');
    if (pos == std::string::npos)
    {
        start_offset = std::strtoull(token.c_str(), nullptr, 10);
        return;
    }
    const std::string d = token.substr(0, pos);
    const std::string o = token.substr(pos + 1);
    if (!d.empty())
        split_depth = std::atoi(d.c_str());
    if (!o.empty())
        start_offset = std::strtoull(o.c_str(), nullptr, 10);
}

bool Traversal::replay_path(Game &g, const std::vector<std::size_t> &path, int &max_depth_reached)
{
    int depth = 0;
    for (std::size_t idx : path)
    {
        const std::size_t choices = g.move_count();
        if (choices == 0 || idx >= choices)
            return false;
        g.select_move(idx);
        ++depth;
    }
    if (depth > max_depth_reached)
        max_depth_reached = depth;
    return true;
}

void Traversal::enumerate_frontier(const Game &base,
                                   int split_depth,
                                   std::uint64_t skip,
                                   std::uint64_t limit,
                                   std::vector<std::vector<std::size_t>> &out,
                                   std::uint64_t &total_frontier_count)
{
    out.clear();
    total_frontier_count = 0;
    std::vector<std::size_t> path;
    bool stop = false;

    std::function<void(const Game &, int)> dfs = [&](const Game &g, int depth) {
        if (stop)
            return;
        const std::size_t choices = g.move_count();
        if (choices == 0 || depth == split_depth)
        {
            ++total_frontier_count;
            if (total_frontier_count > skip && (limit == 0 || out.size() < limit))
            {
                out.push_back(path);
            }
            if (limit != 0 && out.size() >= limit)
            {
                total_frontier_count = skip + out.size();
                stop = true;
            }
            return;
        }
        for (std::size_t i = 0; i < choices; ++i)
        {
            if (stop)
                break;
            Game next = g;
            next.select_move(i);
            path.push_back(i);
            dfs(next, depth + 1);
            path.pop_back();
        }
    };

    dfs(base, 0);
}

void Traversal::dfs_to_terminal(Game &g,
                                std::uint64_t &nodes,
                                std::uint64_t &leaves,
                                int current_depth,
                                int &max_depth_reached,
                                int max_depth_limit,
                                const std::chrono::steady_clock::time_point *deadline,
                                std::atomic<bool> *stop,
                                const std::uint64_t node_cap,
                                const std::uint64_t leaf_cap)
{
    if (stop && stop->load(std::memory_order_relaxed)) return;
    if (deadline && std::chrono::steady_clock::now() >= *deadline)
    {
        if (stop) stop->store(true, std::memory_order_relaxed);
        return;
    }
    const std::size_t choices = g.move_count();
    if (choices == 0)
    {
        ++leaves;
        if (current_depth > max_depth_reached)
            max_depth_reached = current_depth;
        if ((leaf_cap && leaves >= leaf_cap) || (stop && stop->load()))
        {
            if (stop) stop->store(true, std::memory_order_relaxed);
            return;
        }
        return;
    }
    if (max_depth_limit > 0 && current_depth >= max_depth_limit)
    {
        ++leaves;
        if (current_depth > max_depth_reached)
            max_depth_reached = current_depth;
        if ((leaf_cap && leaves >= leaf_cap) || (stop && stop->load()))
        {
            if (stop) stop->store(true, std::memory_order_relaxed);
            return;
        }
        return;
    }
    ++nodes;
    if ((node_cap && nodes >= node_cap))
    {
        if (stop) stop->store(true, std::memory_order_relaxed);
        return;
    }
    for (std::size_t i = 0; i < choices; ++i)
    {
        if (stop && stop->load(std::memory_order_relaxed)) return;
        if (deadline && std::chrono::steady_clock::now() >= *deadline)
        {
            if (stop) stop->store(true, std::memory_order_relaxed);
            return;
        }
        Game next = g;
        next.select_move(i);
        dfs_to_terminal(next, nodes, leaves, current_depth + 1, max_depth_reached, max_depth_limit, deadline, stop, node_cap, leaf_cap);
    }
}

Traversal::Result Traversal::run(const Options& opts)
{
    Result res{};

    int split_depth = opts.split_depth;
    std::uint64_t start_offset = opts.start_offset;
    if (!opts.seed_token.empty())
    {
        apply_seed_token(opts.seed_token, split_depth, start_offset);
    }

    Game base;

    std::vector<std::vector<std::size_t>> frontier;
    enumerate_frontier(base, split_depth, start_offset, opts.max_frontier, frontier, res.total_frontier);

    const std::size_t N = frontier.size();
    if (opts.verbose)
    {
        std::cout << "Frontier split_depth=" << split_depth
                  << ", built window size=" << N
                  << ", total_frontier≥" << res.total_frontier
                  << ", start_offset=" << start_offset << "\n";
    }

    std::atomic<std::uint64_t> nodes{0};
    std::atomic<std::uint64_t> leaves{0};
    std::atomic<std::uint64_t> seeds_done{0};
    int max_depth_reached = 0;
    std::atomic<bool> stop{false};
    std::chrono::steady_clock::time_point deadline;
    const std::chrono::steady_clock::time_point *deadline_ptr = nullptr;
    if (opts.time_limit.count() > 0)
    {
        deadline = std::chrono::steady_clock::now() + opts.time_limit;
        deadline_ptr = &deadline;
    }

#ifdef _OPENMP
#pragma omp parallel
    {
        std::uint64_t local_nodes = 0;
        std::uint64_t local_leaves = 0;
        int local_max_depth = 0;

#pragma omp for schedule(dynamic)
        for (std::int64_t i = 0; i < static_cast<std::int64_t>(N); ++i)
        {
            Game g;
            if (!replay_path(g, frontier[static_cast<std::size_t>(i)], local_max_depth))
                continue;

            if (!stop.load(std::memory_order_relaxed))
            {
                dfs_to_terminal(g, local_nodes, local_leaves,
                                static_cast<int>(frontier[static_cast<std::size_t>(i)].size()),
                                local_max_depth, opts.max_depth,
                                deadline_ptr, &stop,
                                opts.node_limit ? (opts.node_limit - nodes.load(std::memory_order_relaxed)) : 0,
                                opts.leaf_limit ? (opts.leaf_limit - leaves.load(std::memory_order_relaxed)) : 0);
                seeds_done.fetch_add(1, std::memory_order_relaxed);
            }
        }

        nodes.fetch_add(local_nodes, std::memory_order_relaxed);
        leaves.fetch_add(local_leaves, std::memory_order_relaxed);
#pragma omp critical
        {
            if (local_max_depth > max_depth_reached)
                max_depth_reached = local_max_depth;
        }
    }
#else
    {
        std::uint64_t seq_nodes = 0;
        std::uint64_t seq_leaves = 0;
        std::uint64_t seq_done = 0;
        for (std::size_t i = 0; i < N; ++i)
        {
            Game g;
            if (!replay_path(g, frontier[i], max_depth_reached))
                continue;
            if (!stop.load(std::memory_order_relaxed))
            {
                dfs_to_terminal(g, seq_nodes, seq_leaves,
                                static_cast<int>(frontier[i].size()), max_depth_reached,
                                opts.max_depth, deadline_ptr, &stop,
                                opts.node_limit ? (opts.node_limit - seq_nodes) : 0,
                                opts.leaf_limit ? (opts.leaf_limit - seq_leaves) : 0);
                ++seq_done;
            }
            if (stop.load(std::memory_order_relaxed)) break;
        }
        nodes.store(seq_nodes, std::memory_order_relaxed);
        leaves.store(seq_leaves, std::memory_order_relaxed);
        seeds_done.store(seq_done, std::memory_order_relaxed);
    }
#endif

    res.nodes_visited = nodes.load();
    res.terminal_positions = leaves.load();
    res.max_depth_reached = max_depth_reached;
    res.processed_seeds = seeds_done.load(std::memory_order_relaxed);
    res.next_offset = start_offset + res.processed_seeds;
    {
        std::ostringstream oss;
        oss << split_depth << ":" << res.next_offset;
        res.resume_token = oss.str();
    }
    return res;
}
