#include "graphics/texture.h"

#include <string>

#include "graphics/opengl.h"
#include "graphics/sampler.h"
#include "graphics/texture_data.h"
#include "utils/auto_release.h"
#include "utils/exception.h"
#include "utils/formatter.h"

namespace
{
    auto to_opengl(ufps::TextureFormat format, bool include_size) -> ::GLenum
    {
        switch (format)
        {
            using enum ufps::TextureFormat;
        case R:
            return include_size ? GL_R8 : GL_RED;
        case RGB:
            return include_size ? GL_RGB8 : GL_RGB;
        case RGBA:
            return include_size ? GL_RGBA8 : GL_RGB;

        default:
            throw ufps::Exception("unknown texture format: {}", format);
        }
    }
}

namespace ufps
{
    Texture::Texture(const TextureData &texture, const std::string &name, const Sampler &sampler)
        : _handle{0u, [](auto texture)
                  { ::glDeleteTextures(1u, &texture); }},
          _bindless_handle{},
          _name{name},
          _width{texture.width},
          _height{texture.height}
    {
        ::glCreateTextures(GL_TEXTURE_2D, 1, &_handle);
        ::glObjectLabel(GL_TEXTURE, _handle, name.length(), name.data());

        ::glTextureStorage2D(_handle, 1, to_opengl(texture.format, true), texture.width, texture.height);
        ::glTextureSubImage2D(_handle, 0, 0, 0, texture.width, texture.height, to_opengl(texture.format, false), GL_UNSIGNED_BYTE, texture.data.data());

        _bindless_handle = ::glGetTextureSamplerHandleARB(_handle, sampler.native_handle());
        ::glMakeTextureHandleResitentARB(_bindless_handle);
    }

    Texture::~Texture()
    {
        if (_handle)
        {
            ::glMakeTextureHandleNonResitentARB(_bindless_handle);
        }
    }

    auto Texture::native_handle() const -> ::GLuint64
    {
        return _handle;
    }

    auto Texture::name() const -> std::string
    {
        return _name;
    }

    auto Texture::width() const -> std::uint32_t
    {
        return _width;
    }
    auto Texture::height() const -> std::uint32_t
    {
        return _height;
    }

}