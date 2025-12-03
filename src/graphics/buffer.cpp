#include "graphics/buffer.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "graphics/opengl.h"
#include "utils/auto_release.h"
#include "utils/ensure.h"

namespace ufps
{
    Buffer::Buffer(std::size_t size, std::string_view name)
        : _buffer{0u, [](auto vbo)
                  { ::glDeleteBuffers(1, &vbo); }},
          _size(size)
    {
        ensure(size > 0, "Cannot create Buffor of size <= 0");
        ::glCreateBuffers(1, &_buffer);
        ::glNamedBufferStorage(_buffer, size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        ::glObjectLabel(GL_BUFFER, _buffer, name.length(), name.data());
    }
    auto Buffer::write(DataBufferView data, std::size_t offset) const -> void
    {
        expect(_size >= data.size_bytes() + offset, "buffer to small");
        ::glNamedBufferSubData(_buffer, offset, data.size(), data.data());
    }

    auto Buffer::native_handle() const -> ::GLuint
    {
        return _buffer;
    }

}