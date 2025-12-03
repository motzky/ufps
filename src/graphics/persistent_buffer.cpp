#include "graphics/persistent_buffer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "graphics/opengl.h"
#include "utils/auto_release.h"
#include "utils/data_buffer.h"
#include "utils/ensure.h"

namespace ufps
{
    PersistentBuffer::PersistentBuffer(std::size_t size)
        : _buffer{0u, [](auto buffer)
                  { ::glUnmapNamedBuffer(buffer); ::glDeleteBuffers(1, &buffer); }},
          _size{size},
          _map{}
    {
        ::glCreateBuffers(1, &_buffer);
        ::glNamedBufferStorage(_buffer, size, nullptr, GL_DYNAMIC_STORAGE_BIT);

        ::glMapNamedBuffer(_buffer, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    }
    auto PersistentBuffer::write(DataBufferView data, std::size_t offset) const -> void
    {
        expect(_size >= data.size_bytes() + offset, "buffer to small");
        std::memcpy(reinterpret_cast<std::byte *>(_map) + offset, data.data(), data.size_bytes());
    }

    auto PersistentBuffer::native_handle() const -> ::GLuint
    {
        return _buffer;
    }

}