#pragma once

#include <string>

#include "graphics/opengl.h"
#include "graphics/sampler.h"
#include "graphics/texture_data.h"
#include "utils/auto_release.h"

namespace ufps
{
    class Texture
    {
    public:
        Texture(const TextureData &texture, const std::string &name, const Sampler &sampler);
        ~Texture();
        Texture(Texture &&) = default;
        auto operator=(Texture &&) -> Texture & = default;

        auto bindless_handle() const -> ::GLuint64;
        auto native_handle() const -> ::GLuint;

        auto name() const -> std::string;

        auto width() const -> std::uint32_t;
        auto height() const -> std::uint32_t;

    private:
        AutoRelease<::GLuint> _handle;
        ::GLuint64 _bindless_handle;
        std::string _name;
        std::uint32_t _width;
        std::uint32_t _height;
    };
}