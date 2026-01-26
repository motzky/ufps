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
    PersistentBuffer::PersistentBuffer(std::size_t size, std::string_view name)
        : _buffer{0u, [](auto buffer)
                  { ::glUnmapNamedBuffer(buffer); ::glDeleteBuffers(1, &buffer); }},
          _size{size},
          _map{},
          _name{name}
    {
        const auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        ::glCreateBuffers(1, &_buffer);
        ::glNamedBufferStorage(_buffer, size, nullptr, GL_DYNAMIC_STORAGE_BIT | flags);

        _map = ::glMapNamedBufferRange(_buffer, 0, _size, flags);
        ::glObjectLabel(GL_BUFFER, _buffer, name.length(), name.data());
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
    auto PersistentBuffer::name() const -> std::string_view
    {
        return _name;
    }

    auto PersistentBuffer::to_string() const -> std::string
    {
        return std::format("{} {}", _name, _size);
    }

}