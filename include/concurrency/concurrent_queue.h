#pragma once

#include <atomic>
#include <cstdint>
#include <queue>

#include "concurrency/cond_var.h"
#include "concurrency/lock.h"

namespace ufps
{

    template <class T, class Q = std::queue<T>>
    class ConcurrentQueue
    {
    public:
        auto pop() -> T
        {
            const auto lock = std::scoped_lock{_lock};
            --_size;

            auto obj = std::move(_q.front());
            _q.pop();

            return obj;
        }

        auto push(T &&obj) -> void
        {
            const auto lock = std::scoped_lock{_lock};
            ++_size;
            _q.push(std::forward<T>(obj));
        }

        auto empty() const -> bool
        {
            return _size == 0u;
        }

        auto size() const -> std::uint32_t
        {
            return _size;
        }

        auto yield() -> Q
        {
            auto q = Q{};

            {
                const auto lock = std::scoped_lock{_lock};
                std::ranges::swap(q, _q);
                _size = 0u;
            }

            return q;
        }

    private:
        Q _q;
        Lock<> _lock;
        CondVar _cv;
        std::atomic<std::uint32_t> _size;
    };

}