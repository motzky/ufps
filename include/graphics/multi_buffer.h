#pragma once

#include <string_view>

#include "utils/data_buffer.h"
#include <graphics/utils.h>

namespace ufps
{

    /**
     * A multi buffer wrapper of a Buffer type. Will allocate size * Frames amount of data an can advance through it
     */
    template <IsBuffer Buffer, std::size_t Frames = 3zu>
    class MultiBuffer
    {
    public:
        MultiBuffer(std::size_t size, std::string_view name)
            : _buffer{size * Frames, name},
              _size{size},
              _frame_offset{}
        {
        }

        auto write(DataBufferView data, std::size_t offset) -> void
        {
            _buffer.write(data, offset + _frame_offset);
        }

        auto advance() -> void
        {
            _frame_offset = (_frame_offset + _size) % (_size * Frames);
        }

        auto native_handle() const
        {
            return _buffer.native_handle();
        }

        auto buffer() const -> const Buffer &
        {
            return _buffer;
        }

        auto size() const -> std::size_t
        {
            return _size;
        }

        auto frame_offset_bytes() const -> std::size_t
        {
            return _frame_offset;
        }

        auto name() const -> std::string_view
        {
            return _buffer.name();
        }

    private:
        Buffer _buffer;
        std::size_t _size;
        std::size_t _frame_offset;
    };
}