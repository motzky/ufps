#include "graphics/frame_buffer.h"

#include <algorithm>
#include <cstdint>
#include <ranges>
#include <span>

#include "graphics/opengl.h"
#include "graphics/texture.h"
#include "utils/auto_release.h"
#include "utils/ensure.h"

namespace ufps
{
    FrameBuffer::FrameBuffer(std::span<const Texture *> color_textures, const Texture *depth_texture)
        : _handle{0u, [](const auto buffer)
                  { ::glDeleteFramebuffers(1, &buffer); }},
          _color_textures{color_textures},
          _depth_texture{depth_texture}
    {
        expect(!color_textures.empty(), "must have at least one color texture");
        expect(color_textures.size() < 10u, "only 10 color textures are supported");
        expect(std::ranges::all_of(color_textures, [&](const auto *e)
                                   { return e->width() == color_textures[0]->width() && e->height() == color_textures[0]->height(); }),
               "all color texture must have same dimensions");
        expect(depth_texture != nullptr, "must have a depth texture");

        ::glCreateFramebuffers(1, &_handle);

        for (const auto &[index, tex] : std::views::enumerate(_color_textures))
        {
            ::glNamedFramebufferTexture(_handle, static_cast<::GLenum>(GL_COLOR_ATTACHMENT0 + index), tex->native_handle(), 0);
        }

        ::glNamedFramebufferTexture(_handle, GL_DEPTH_ATTACHMENT, depth_texture->native_handle(), 0);

        const auto attachments = std::views::iota(0zu, _color_textures.size()) |
                                 std::views::transform([](auto e)
                                                       { return static_cast<::GLenum>(GL_COLOR_ATTACHMENT0 + e); }) |
                                 std::ranges::to<std::vector>();

        ::glNamedFramebufferDrawBuffers(_handle, static_cast<::GLsizei>(attachments.size()), attachments.data());

        auto status = ::glCheckNamedFramebufferStatus(_handle, GL_FRAMEBUFFER);
        expect(status == GL_FRAMEBUFFER_COMPLETE, "framebuffer is not complete");
    }

    auto FrameBuffer::native_handle() const -> ::GLuint
    {
        return _handle;
    }

    auto FrameBuffer::bind() const -> void
    {
        ::glBindFramebuffer(GL_FRAMEBUFFER, _handle);
    }

    auto FrameBuffer::unbind() const -> void
    {
        ::glBindFramebuffer(GL_FRAMEBUFFER, 0u);
    }

    auto FrameBuffer::width() const -> std::uint32_t
    {
        return _color_textures[0]->width();
    }

    auto FrameBuffer::height() const -> std::uint32_t
    {
        return _color_textures[0]->height();
    }

    auto FrameBuffer::color_textures() const -> std::span<const Texture *>
    {
        return _color_textures;
    }

}