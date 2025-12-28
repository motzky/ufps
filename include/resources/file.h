#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <ranges>
#include <string_view>

#ifdef WIN32
#include <Windows.h>
#else
#include <limits>
#include <sys/mman.h>
#endif

#include "utils/auto_release.h"
#include "utils/ensure.h"

namespace ufps
{
    enum class CreationMode
    {
        OPEN,
        CREATE
    };

    class File
    {
    public:
        File(const std::filesystem::path &path, CreationMode mode = CreationMode::OPEN);
        auto as_string() const -> std::string_view;
        auto as_bytes() const -> std::span<const std::byte>;

        template <class T>
        auto write(const T &data) -> void
            requires(sizeof(std::ranges::range_value_t<T>) == 1)
        {
#ifdef _WIN32
            std::memcpy(_map_view.get(), std::ranges::data(data), std::ranges::size(data));
            ensure(::FlushViewOfFile(_map_view.get(), data.size()) != 0, "failed to flush file");
#else
            resize_and_remap(std::ranges::size(data));
            std::memcpy(_map_view, std::ranges::data(data), std::ranges::size(data));
#endif
            get_file_size();
        }

    private:
#ifdef _WIN32
        using FileHandle = HANDLE;
        AutoRelease<HANDLE, INVALID_HANDLE_VALUE> _handle;
        AutoRelease<HANDLE, reinterpret_cast<HANDLE>(NULL)> _mapping;
        std::unique_ptr<void, decltype(&::UnmapViewOfFile)> _map_view;
        std::size_t _filesize;

#else
        using FileHandle = int;
        AutoRelease<int, -1> _handle;
        AutoRelease<void *, nullptr> _map_view;
        std::size_t _filesize;
        CreationMode _mode;
        bool _mapped;

        auto resize_and_remap(std::size_t new_size) -> void;
#endif
        auto get_file_size() -> void;
    };
}
