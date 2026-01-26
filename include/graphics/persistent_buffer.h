#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "graphics/opengl.h"
#include "utils/auto_release.h"
#include "utils/data_buffer.h"

namespace ufps
{
    class PersistentBuffer
    {
    public:
        PersistentBuffer(std::size_t size, std::string_view name);

        auto write(DataBufferView data, std::size_t offset) const -> void;

        auto native_handle() const -> ::GLuint;

        auto name() const -> std::string_view;

        auto to_string() const -> std::string;

    private:
        AutoRelease<::GLuint> _buffer;
        std::size_t _size;
        void *_map;
        std::string _name;
    };
}