#pragma once

#include <mutex>

namespace ufps
{
    template <class M = std::mutex>
    class Lock
    {
    public:
        auto lock() -> void { _lock.lock(); }

        auto unlock() -> void { _lock.unlock(); }

    private:
        M _lock;
    };
}