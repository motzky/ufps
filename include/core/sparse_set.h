#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <ranges>
#include <type_traits>
#include <vector>

#include "utils/ensure.h"

namespace ufps
{
    template <class T, class Allocator = std::allocator<T>>
    class SparseSet
    {
        class Handle
        {
            inline static constexpr auto Invalid = std::numeric_limits<std::size_t>::max();

            constexpr Handle()
                : Handle(Invalid)
            {
            }

            constexpr explicit Handle(std::size_t index)
                : _index{index}
            {
            }

            constexpr auto operator<=>(const Handle &) const = default;

            std::size_t _index;

            friend SparseSet;
        };

    public:
        using value_type = T;
        using handle_type = Handle;

        constexpr SparseSet();

        template <class... Args>
        constexpr auto emplace(Args &&...args) -> handle_type
        {
            const auto dense_index = std::ranges::size(_data);
            _data.emplace_back(std::forward<Args>(args)...);

            const auto sparse_index = std::ranges::size(_sparse);
            _sparse.push_back(dense_index);

            _dense.push_back(sparse_index);

            return handle_type{sparse_index};
        }

        template <class Self>
        constexpr auto operator[](this Self &&self, handle_type handle)
        {
#if defined(__GNUC__) && (__GNUC__ <= 15)
            using RetType = std::conditional_t<std::is_const_v<std::remove_reference<Self>>, const value_type, value_type>;
#else
            using RetType = std::conditional_t<std::is_const_v<std::remove_reference<Self>>, const value_type &, value_type &>;
#endif

            if (std::ranges::empty(self._dense))
            {
                return std::optional<RetType>{};
            }

            const auto sparse_index = handle._index;
            if (sparse_index >= std::ranges::size(self._sparse))
            {
                return std::optional<RetType>{};
            }

            const auto dense_index = self._sparse[sparse_index];
            if (dense_index >= std::ranges::size(self._dense))
            {
                return std::optional<RetType>{};
            }

            if (self._dense[dense_index] != sparse_index)
            {
                return std::optional<RetType>{};
            }

            return std::optional<RetType>(self._data[dense_index]);
        }

        constexpr auto remove(handle_type handle);
        constexpr auto size() const -> std::size_t;

        constexpr auto empty() const -> bool;

        constexpr auto handles() const -> std::vector<handle_type>;

        constexpr auto data() const -> std::span<const T>;

    private:
        template <class U>
        using VectorRebind = std::vector<U, typename std::allocator_traits<Allocator>::template rebind_alloc<std::size_t>>;

        VectorRebind<std::size_t> _sparse;
        VectorRebind<std::size_t> _dense;
        std::vector<T, Allocator> _data;
    };

    template <class T, class Allocator>
    constexpr SparseSet<T, Allocator>::SparseSet()
        : _sparse{},
          _dense{},
          _data{}
    {
    }

    template <class T, class Allocator>
    constexpr auto SparseSet<T, Allocator>::size() const -> std::size_t
    {
        return std::ranges::size(_data);
    }

    template <class T, class Allocator>
    constexpr auto SparseSet<T, Allocator>::empty() const -> bool
    {
        return std::ranges::empty(_data);
    }

    template <class T, class Allocator>
    constexpr auto SparseSet<T, Allocator>::remove(handle_type handle)
    {
        const auto sparse_index = handle._index;
        ensure(sparse_index < std::ranges::size(_sparse), "invalid handle: {}", sparse_index);

        const auto dense_index = _sparse[sparse_index];
        ensure(dense_index < std::ranges::size(_dense), "invalid handle: {}", sparse_index);

        ensure(_dense[dense_index] == sparse_index, "invalid handle: {}", sparse_index);

        if (dense_index == std::ranges::size(_data) - 1zu)
        {
            _data.pop_back();
            _dense.pop_back();
            _sparse[sparse_index] = handle_type::Invalid;

            return;
        }

        std::ranges::swap(_data[_sparse[sparse_index]], *(std::ranges::end(_data) - 1zu));
        _data.pop_back();
        std::ranges::swap(_dense[_sparse[sparse_index]], *(std::ranges::end(_dense) - 1zu));
        _dense.pop_back();

        _sparse[sparse_index] = handle_type::Invalid;
        if (!std::ranges::empty(_dense))
        {
            _sparse[std::ranges::size(_sparse) - 1zu] = dense_index;
        }
    }

    template <class T, class Allocator>
    constexpr auto SparseSet<T, Allocator>::handles() const -> std::vector<handle_type>
    {
        return _sparse |
               std::views::filter([](const auto &e)
                                  { return e != handle_type::Invalid; }) |
               std::views::transform([](const auto &e)
                                     { return handle_type{e}; }) |
               std::ranges::to<std::vector>();
    }

    template <class T, class Allocator>
    constexpr auto SparseSet<T, Allocator>::data() const -> std::span<const T>
    {
        return _data;
    }
}