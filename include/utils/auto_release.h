#pragma once

#include <functional>
#include <ranges>
#include <utility>

namespace ufps
{
    template <class T, T Invalid = T()>
        requires std::is_trivially_copyable_v<T>
    class AutoRelease
    {
    public:
        constexpr AutoRelease();
        constexpr AutoRelease(T obj, std::function<void(T)> deleter);

        constexpr ~AutoRelease();
        AutoRelease(const AutoRelease &) = delete;
        auto operator=(const AutoRelease &) -> AutoRelease & = delete;

        constexpr AutoRelease(AutoRelease &&other);

        constexpr auto operator=(AutoRelease &&other) -> AutoRelease &;

        constexpr auto swap(AutoRelease &other) noexcept -> void;
        constexpr auto reset(T obj) -> void;
        constexpr T get() const;
        constexpr operator T() const;
        constexpr explicit operator bool() const;
        constexpr T *operator&() noexcept;

    private:
        T _obj;
        std::function<void(T)> _deleter;
    };

    template <class T, T Invalid>
        requires std::is_trivially_copyable_v<T>
    constexpr AutoRelease<T, Invalid>::AutoRelease()
        : AutoRelease(Invalid, nullptr)
    {
    }

    template <class T, T Invalid>
        requires std::is_trivially_copyable_v<T>
    constexpr AutoRelease<T, Invalid>::AutoRelease(T obj, std::function<void(T)> deleter)
        : _obj(obj),
          _deleter(std::move(deleter))
    {
    }

    template <class T, T Invalid>
        requires std::is_trivially_copyable_v<T>
    constexpr AutoRelease<T, Invalid>::~AutoRelease()
    {
        if ((_obj != Invalid) && _deleter)
        {
            _deleter(_obj);
        }
    }

    template <class T, T Invalid>
        requires std::is_trivially_copyable_v<T>
    constexpr AutoRelease<T, Invalid>::AutoRelease(AutoRelease &&other)
        : AutoRelease()
    {
        swap(other);
    }

    template <class T, T Invalid>
        requires std::is_trivially_copyable_v<T>
    constexpr auto AutoRelease<T, Invalid>::operator=(AutoRelease &&other) -> AutoRelease &
    {
        AutoRelease new_obj{std::move(other)};
        swap(new_obj);
        return *this;
    }

    template <class T, T Invalid>
        requires std::is_trivially_copyable_v<T>
    constexpr auto AutoRelease<T, Invalid>::swap(AutoRelease &other) noexcept -> void
    {
        std::ranges::swap(_obj, other._obj);
        std::ranges::swap(_deleter, other._deleter);
    }

    template <class T, T Invalid>
        requires std::is_trivially_copyable_v<T>
    constexpr auto AutoRelease<T, Invalid>::reset(T obj) -> void
    {
        if ((_obj != Invalid) && _deleter)
        {
            _deleter(_obj);
        }

        _obj = obj;
    }

    template <class T, T Invalid>
        requires std::is_trivially_copyable_v<T>
    constexpr T AutoRelease<T, Invalid>::get() const
    {
        return _obj;
    }

    template <class T, T Invalid>
        requires std::is_trivially_copyable_v<T>
    constexpr AutoRelease<T, Invalid>::operator T() const
    {
        return _obj;
    }

    template <class T, T Invalid>
        requires std::is_trivially_copyable_v<T>
    constexpr AutoRelease<T, Invalid>::operator bool() const
    {
        return _obj != Invalid;
    }

    template <class T, T Invalid>
        requires std::is_trivially_copyable_v<T>
    constexpr T *AutoRelease<T, Invalid>::operator&() noexcept
    {
        return std::addressof(_obj);
    }
}
