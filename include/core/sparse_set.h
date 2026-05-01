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
            inline static constexpr auto Invalid = std::numeric_limits<std::uint32_t>::max();

            constexpr Handle()
                : Handle(Invalid)
            {
            }

            constexpr explicit Handle(std::uint32_t index, std::uint32_t version)
                : _index{index},
                  _version{version}
            {
            }

            constexpr auto operator<=>(const Handle &) const = default;

            std::uint32_t _index;
            std::uint32_t _version;

            friend SparseSet;
        };

    public:
        using value_type = T;
        using handle_type = Handle;

        constexpr SparseSet();

        template <class... Args>
        constexpr auto emplace(Args &&...args) -> handle_type
        {
            const auto dense_index = static_cast<std::uint32_t>(std::ranges::size(_data));
            _data.emplace_back(std::forward<Args>(args)...);

            auto sparse_index = static_cast<std::uint32_t>(std::ranges::size(_sparse));
            auto version = 0u;

            if (!std::ranges::empty(_free))
            {
                sparse_index = _free.back();
                _free.pop_back();

                _sparse[sparse_index]._index = dense_index;
                ++_sparse[sparse_index]._version;
                version = _sparse[sparse_index]._version;
            }
            else
            {
                _sparse.push_back(handle_type{dense_index, version});
            }

            _dense.push_back(sparse_index);

            return handle_type{sparse_index, version};
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

            const auto dense_index = self._sparse[sparse_index]._index;
            if (dense_index >= std::ranges::size(self._dense))
            {
                return std::optional<RetType>{};
            }

            if (self._dense[dense_index] != sparse_index || handle._version != self._sparse[sparse_index]._version)
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
        using VectorRebind = std::vector<U, typename std::allocator_traits<Allocator>::template rebind_alloc<U>>;

        VectorRebind<handle_type> _sparse;
        VectorRebind<std::uint32_t> _dense;
        std::vector<T, Allocator> _data;
        VectorRebind<std::size_t> _free;
    };

    template <class T, class Allocator>
    constexpr SparseSet<T, Allocator>::SparseSet()
        : _sparse{},
          _dense{},
          _data{},
          _free{}
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

        const auto dense_index = _sparse[sparse_index]._index;
        ensure(dense_index < std::ranges::size(_dense), "invalid handle: {}", sparse_index);

        ensure(_dense[dense_index] == sparse_index, "invalid handle: {}", sparse_index);

        if (dense_index == std::ranges::size(_data) - 1zu)
        {
            _data.pop_back();
            _dense.pop_back();
            _sparse[sparse_index]._index = handle_type::Invalid;
            _free.push_back(sparse_index);

            return;
        }

        std::ranges::swap(_data[_sparse[sparse_index]._index], *(std::ranges::end(_data) - 1u));
        _data.pop_back();
        std::ranges::swap(_dense[_sparse[sparse_index]._index], *(std::ranges::end(_dense) - 1u));
        _dense.pop_back();

        _sparse[sparse_index]._index = handle_type::Invalid;
        _free.push_back(sparse_index);
        if (!std::ranges::empty(_dense))
        {
            _sparse[std::ranges::size(_sparse) - 1u]._index = dense_index;
        }
    }

    template <class T, class Allocator>
    constexpr auto SparseSet<T, Allocator>::handles() const -> std::vector<handle_type>
    {
        return _sparse |
               std::views::filter([](const auto &e)
                                  { return e._index != handle_type::Invalid; }) |
               std::ranges::to<std::vector>();
    }

    template <class T, class Allocator>
    constexpr auto SparseSet<T, Allocator>::data() const -> std::span<const T>
    {
        return _data;
    }
}