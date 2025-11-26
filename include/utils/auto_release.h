#pragma once

#include <functional>
#include <ranges>

namespace ufps
{
    template <class T, T Invalid = T()>
    class AutoRelease
    {
    public:
        AutoRelease()
            : AutoRelease(Invalid, nullptr)
        {
        }

        AutoRelease(T obj, std::function<void(T)> deleter)
            : _obj(obj),
              _deleter(std::move(deleter))
        {
        }

        ~AutoRelease()
        {
            if ((_obj != Invalid) && _deleter)
            {
                _deleter(_obj);
            }
        }

        AutoRelease(const AutoRelease &) = delete;
        auto operator=(const AutoRelease &) -> AutoRelease & = delete;

        AutoRelease(AutoRelease &&other)
            : AutoRelease()
        {
            swap(other);
        }
        auto operator=(AutoRelease &&other) -> AutoRelease &
        {
            AutoRelease new_obj{std::move(other)};
            swap(new_obj);
            return *this;
        }

        auto swap(AutoRelease &other) noexcept -> void
        {
            std::ranges::swap(_obj, other._obj);
            std::ranges::swap(_deleter, other._deleter);
        }

        auto reset(T obj) -> void
        {
            if ((_obj != Invalid) && _deleter)
            {
                _deleter(_obj);
            }

            _obj = obj;
        }

        T get() const { return _obj; }
        operator T() const { return _obj; }

        explicit operator bool() const
        {
            return _obj != Invalid;
        }

        T *operator&() noexcept
        {
            return std::addressof(_obj);
        }

    private:
        T _obj;
        std::function<void(T)> _deleter;
    };
}
