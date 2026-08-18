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

        constexpr auto insert(std::string_view name, T object) -> handle_type;

        constexpr auto operator[](std::string_view name) -> handle_type;
        constexpr auto operator[](handle_type handle);

    private:
        SparseSet<T> _objects;
        StringUnorderedMap<handle_type> _object_names;
    };

    template <class T>
    constexpr auto StorageManager<T>::insert(std::string_view name, T object) -> handle_type
    {
        auto handle = _objects.emplace(std::move(object));
        _object_names[std::string{name}] = handle;
        return handle;
    }

    template <class T>
    constexpr auto StorageManager<T>::operator[](std::string_view name) -> handle_type
    {
        auto element = _object_names.find(name);
        contract_assert(element != std::ranges::cend(_object_names));

        return element->second;
    }

    template <class T>
    constexpr auto StorageManager<T>::operator[](handle_type handle)
    {
        return _objects[handle];
    }

}