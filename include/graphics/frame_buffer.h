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

    private:
        AutoRelease<::GLuint> _handle;
        std::uint32_t _width;
        std::uint32_t _height;
        const std::string &_name;
    };
}