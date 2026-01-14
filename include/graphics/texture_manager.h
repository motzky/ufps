#pragma once

#include <cstdint>
#include <vector>

#include "graphics/buffer.h"
#include "graphics/opengl.h"
#include "graphics/texture.h"

namespace ufps
{
    class TextureManager
    {
    public:
        TextureManager();

        auto add(Texture texture) -> std::uint32_t;
        auto add(std::vector<Texture> textures) -> std::uint32_t;

        auto native_handle() -> ::GLuint;

        auto texture(const std::uint32_t indiex) const -> const Texture *;
        auto textures(const std::vector<std::uint32_t> &indices) const -> std::vector<const Texture *>;

    private:
        Buffer _gpu_buffer;
        std::vector<::GLuint64> _cpu_buffer;
        std::vector<Texture> _textures;
    };
}