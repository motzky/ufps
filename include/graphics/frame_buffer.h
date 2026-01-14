#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "opengl.h"
#include "texture.h"
#include "utils/auto_release.h"

namespace ufps
{
    class FrameBuffer
    {
    public:
        FrameBuffer(std::vector<const Texture *> color_textures, const Texture *depth_texture, const std::string &name);

        auto native_handle() const -> ::GLuint;
        auto name() const -> std::string;

        auto bind() const -> void;
        auto unbind() const -> void;

        auto width() const -> std::uint32_t;
        auto height() const -> std::uint32_t;

        auto color_textures() const -> std::span<const Texture *const>;

    private:
        AutoRelease<::GLuint> _handle;
        std::vector<const Texture *> _color_textures;
        const Texture *_depth_texture;
        const std::string &_name;
    };
}