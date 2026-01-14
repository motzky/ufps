#include "graphics/texture_manager.h"

#include <cstdint>
#include <ranges>
#include <span>
#include <string_view>

#include "graphics/buffer.h"
#include "graphics/opengl.h"
#include "graphics/texture.h"
#include "graphics/utils.h"

using namespace std::literals;

namespace ufps
{
    TextureManager::TextureManager()
        : _gpu_buffer{sizeof(::GLuint64), "bindless_textures"},
          _cpu_buffer{},
          _textures{}
    {
    }

    auto TextureManager::add(Texture texture) -> std::uint32_t
    {
        const auto new_index = _textures.size();

        auto &new_tex = _textures.emplace_back(std::move(texture));

        _cpu_buffer.push_back(new_tex.bindless_handle());

        resize_gpu_buffer(_cpu_buffer, _gpu_buffer, "bindless_textures"sv);

        _gpu_buffer.write(std::as_bytes(std::span{_cpu_buffer.data(), _cpu_buffer.size()}), 0zu);

        return static_cast<std::uint32_t>(new_index);
    }

    auto TextureManager::add(std::vector<Texture> textures) -> std::uint32_t
    {
        const auto new_index = _textures.size();

        _textures.append_range(std::views::as_rvalue(textures));

        _cpu_buffer = _textures |
                      std::views::transform([](auto &e)
                                            { return e.bindless_handle(); }) |
                      std::ranges::to<std::vector>();

        resize_gpu_buffer(_cpu_buffer, _gpu_buffer, "bindless_textures"sv);

        _gpu_buffer.write(std::as_bytes(std::span{_cpu_buffer.data(), _cpu_buffer.size()}), 0zu);

        return static_cast<std::uint32_t>(new_index);
    }

    auto TextureManager::native_handle() -> ::GLuint
    {
        return _gpu_buffer.native_handle();
    }

}