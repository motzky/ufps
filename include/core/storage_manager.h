#pragma once

#include "core/sparse_set.h"
#include "utils/ensure.h"
#include "utils/string_unordered_map.h"

namespace ufps
{
    template <class T>
    class StorageManager
    {
    public:
        using handle_type = SparseSet<T>::handle_type;

        constexpr auto insert(T object) -> handle_type;

        constexpr auto operator[](handle_type handle);

        constexpr auto remove(handle_type handle) -> void;
        constexpr auto data();
        constexpr auto handles();

    private:
        SparseSet<T> _objects;
    };

    template <class T>
    constexpr auto StorageManager<T>::insert(T object) -> handle_type
    {
        auto handle = _objects.emplace(std::move(object));
        return handle;
    }

    template <class T>
    constexpr auto StorageManager<T>::operator[](handle_type handle)
    {
        return _objects[handle];
    }

    template <class T>
    constexpr auto StorageManager<T>::remove(handle_type handle) -> void
    {
        _objects.remove(handle);
    }

    template <class T>
    constexpr auto StorageManager<T>::data()
    {
        return _objects.data();
    }

    template <class T>
    constexpr auto StorageManager<T>::handles()
    {
        return _objects.handles();
    }

}