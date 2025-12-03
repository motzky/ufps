#pragma once

#include <cstddef>
#include <cstdint>

#include "graphics/opengl.h"
#include "utils/auto_release.h"
#include "utils/data_buffer.h"

namespace ufps
{
    class PersistentBuffer
    {
    public:
        PersistentBuffer(std::size_t size);

        auto write(DataBufferView data, std::size_t offset) const -> void;

        auto native_handle() const -> ::GLuint;

    private:
        AutoRelease<::GLuint> _buffer;
        std::size_t _size;
        void *_map;
    };
}