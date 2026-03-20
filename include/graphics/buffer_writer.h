#pragma once

#include <cstddef>
#include <span>

#include "graphics/utils.h"

namespace ufps
{
    template <IsBuffer Buffer>
    class BufferWriter
    {
    public:
        constexpr BufferWriter(Buffer &buffer)
            : _buffer(buffer),
              _offset{}
        {
        }

        template <class T>
        auto write(const T &obj) -> void
            requires(std::is_trivially_copyable_v<T>)
        {
            auto spn = std::span<const T>(&obj, 1);
            write(spn);
        }

        template <class T>
        auto write(std::span<const T> data) -> void
        {
            _buffer.write(std::as_bytes(data), _offset);
            _offset += data.size_bytes();
        }

    private:
        Buffer &_buffer;
        std::size_t _offset;
    };
}