#pragma once

#include <chrono>
#include <coroutine>

#include "concurrency/concurrent_queue.h"
#include "concurrency/thread_pool.h"

namespace ufps
{
    class AwaitableManager
    {
    public:
        AwaitableManager(ThreadPool &pool)
            : _pool{pool},
              _next_tick_queue{}
        {
        }

        ~AwaitableManager()
        {
            auto to_process = _next_tick_queue.yield();
            while (!to_process.empty())
            {
                to_process.front().destroy();
                to_process.pop();
            }
        }

        AwaitableManager(const AwaitableManager &) = delete;
        auto operator=(const AwaitableManager &) -> AwaitableManager & = delete;

        auto operator co_await()
        {
            struct Awaitable
            {
                auto await_ready()
                {
                    return false;
                }

                auto await_suspend(std::coroutine_handle<> h)
                {
                    self._next_tick_queue.push(std::move(h));
                }

                auto await_resume()
                {
                }

                AwaitableManager &self;
            };

            return Awaitable{*this};
        }

        auto operator()(std::chrono::nanoseconds wait_time)
        {
            struct Awaitable
            {
                auto await_ready()
                {
                    return false;
                }

                auto await_suspend(std::coroutine_handle<> h)
                {
                    self._timer_queue.push({.time_point = std::chrono::steady_clock::now() + wait_time,
                                            .handle = h});
                }

                auto await_resume()
                {
                }

                AwaitableManager &self;
                std::chrono::nanoseconds wait_time;
            };

            return Awaitable{*this, wait_time};
        }

        auto pump() -> void
        {
            auto to_process = _next_tick_queue.yield();

            while (!to_process.empty())
            {
                auto handle = std::move(to_process.front());
                to_process.pop();

                _pool.add([handle]
                          { handle.resume(); });
            }

            while (!_timer_queue.empty())
            {
                auto timer_awaitable = _timer_queue.pop();
                if (timer_awaitable.time_point <= std::chrono::steady_clock::now())
                {
                    _pool.add([handle = std::move(timer_awaitable.handle)]
                              { handle.resume(); });
                }
                else
                {
                    _timer_queue.push(std::move(timer_awaitable));
                    break;
                }
            }
        }

    private:
        struct TimerAwaitable
        {
            std::chrono::steady_clock::time_point time_point;
            std::coroutine_handle<> handle;

            auto operator>(const TimerAwaitable &other) const -> bool
            {
                return time_point > other.time_point;
            }
        };
        using TimePriorityQueue = std::priority_queue<TimerAwaitable, std::vector<TimerAwaitable>, std::greater<TimerAwaitable>>;

        ThreadPool &_pool;
        ConcurrentQueue<std::coroutine_handle<>> _next_tick_queue;
        ConcurrentQueue<TimerAwaitable, TimePriorityQueue> _timer_queue;
    };
}