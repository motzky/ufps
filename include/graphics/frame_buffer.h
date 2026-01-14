#pragma once

#include <cstdint>
#include <span>

#include "opengl.h"
#include "texture.h"
#include "utils/auto_release.h"

namespace ufps
{
    class FrameBuffer
    {
    public:
        FrameBuffer(std::span<const Texture *> color_textures, const Texture *depth_texture);

        auto native_handle() const -> ::GLuint;

        auto bind() const -> void;
        auto unbind() const -> void;

        auto width() const -> std::uint32_t;
        auto height() const -> std::uint32_t;

        auto color_textures() const -> std::span<const Texture *>;

    private:
        AutoRelease<::GLuint> _handle;
        std::span<const Texture *> _color_textures;
        const Texture *_depth_texture;
    };
}