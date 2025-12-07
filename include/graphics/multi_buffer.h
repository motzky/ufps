#pragma once

#include <concepts>
#include <cstddef>
#include <string_view>

#include "utils/data_buffer.h"

namespace ufps
{

    template <class T>
    concept IsBuffer = requires(T t, DataBufferView data, std::size_t offset) {
        { t.write(data, offset) };
    };

    /**
     * A multi buffer wrapper of a Buffer type. Will allocate size * Frames amount of data an can advance through it
     */
    template <IsBuffer Buffer, std::size_t Frames = 3zu>
    class MultiBuffer
    {
    public:
        MultiBuffer(std::size_t size, std::string_view name)
            : _buffer{size * Frames, name},
              _original_size{size},
              _frame_offset{}
        {
        }

        auto write(DataBufferView data, std::size_t offset) -> void
        {
            _buffer.write(data, offset + _frame_offset);
        }

        auto advance() -> void
        {
            _frame_offset = (_frame_offset + _original_size) % (_original_size * Frames);
        }

        auto buffer() const -> const Buffer &
        {
            return _buffer;
        }

        auto original_size() const -> std::size_t
        {
            return _original_size;
        }

    private:
        Buffer _buffer;
        std::size_t _original_size;
        std::size_t _frame_offset;
    };
}