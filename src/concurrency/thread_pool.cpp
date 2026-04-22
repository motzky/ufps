#include "concurrency/thread_pool.h"

#include <algorithm>
#include <thread>

#include "concurrency/concurrent_queue.h"
#include "concurrency/cond_var.h"
#include "concurrency/thread.h"
#include "log.h"

namespace ufps
{
    ThreadPool::ThreadPool()
        : ThreadPool(std::clamp(std::thread::hardware_concurrency() - 1u, 1u, 32u))
    {
    }

    ThreadPool::ThreadPool(std::uint32_t worker_count)
        : _worker_count{worker_count},
          _workers{},
          _job_queue{},
          _worker_cv{},
          _job_count{}
    {
        log::info("starting thread poll with {} workers", worker_count);

        for (auto i = 0u; i < worker_count; ++i)
        {
            const auto name = std::format("worker_{}", i);

            _workers.push_back({name, [this](std::stop_token stop_token)
                                { worker(std::move(stop_token)); }});
        }
    }

    ThreadPool::~ThreadPool()
    {
        for (auto &thread : _workers)
        {
            thread.request_stop();
        }
    }

    auto ThreadPool::add(Job job) -> void
    {
        ++_job_count;
        _job_queue.push(std::move(job));
        _worker_cv.notify_one();
    }

    auto ThreadPool::worker_count() -> std::uint32_t
    {
        return _worker_count;
    }

    auto ThreadPool::drain() const -> void
    {
        auto count = _job_count.load();
        while (count != 0u)
        {
            _job_count.wait(count);
            count = _job_count.load();
        }
    }

    auto ThreadPool::worker(std::stop_token stop_token) -> void
    {
        log::info("starting worker thread: {}", std::this_thread::get_id());

        while (!stop_token.stop_requested())
        {
            auto job = Job{};

            {
                auto lock = std::unique_lock{_worker_lock};
                _worker_cv.wait(
                    lock,
                    stop_token,
                    [&]
                    { return !_job_queue.empty(); });

                if (stop_token.stop_requested())
                {
                    break;
                }

                job = _job_queue.pop();
            }

            job();

            if (--_job_count == 0u)
            {
                _job_count.notify_all();
            }
        }
        log::info("ending worker thread: {}", std::this_thread::get_id());
    }
}