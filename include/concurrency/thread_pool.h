#pragma once

#include <atomic>
#include <cstdint>
#include <format>
#include <functional>
#include <vector>

#include "concurrency/concurrent_queue.h"
#include "concurrency/cond_var.h"
#include "concurrency/thread.h"

namespace ufps
{
    using Job = std::move_only_function<void()>;

    class ThreadPool
    {
    public:
        ThreadPool();
        ThreadPool(std::uint32_t worker_count);
        ~ThreadPool();

        auto add(Job job) -> void;

        auto worker_count() -> std::uint32_t;

        auto drain() const -> void;

    private:
        auto worker(std::stop_token stop_token) -> void;

        std::uint32_t _worker_count;
        ConcurrentQueue<Job> _job_queue;
        Lock<> _worker_lock;
        CondVar _worker_cv;
        std::atomic<std::uint32_t> _job_count;
        std::vector<Thread> _workers;
    };
}