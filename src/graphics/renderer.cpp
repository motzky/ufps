#include "graphics/renderer.h"

#include <optional>
#include <ranges>
#include <span>
#include <string_view>

#include "core/camera.h"
#include "core/scene.h"
#include "graphics/command_buffer.h"
#include "graphics/object_data.h"
#include "graphics/opengl.h"
#include "graphics/point_light.h"
#include "graphics/program.h"
#include "graphics/shader.h"
#include "graphics/texture.h"
#include "graphics/texture_data.h"
#include "graphics/texture_manager.h"
#include "graphics/utils.h"
#include "resources/resource_loader.h"
#include "utils/auto_release.h"

using namespace std::literals;

namespace
{
    auto create_program(ufps::ResourceLoader &resource_loader) -> ufps::Program
    {
        const auto sample_vert = ufps::Shader{resource_loader.load_string("shaders/simple.vert"sv), ufps::ShaderType::VERTEX, "sample_vertex_shader"sv};
        const auto sample_frag = ufps::Shader{resource_loader.load_string("shaders/simple.frag"sv), ufps::ShaderType::FRAGMENT, "sample_fragement_shader"sv};

        return ufps::Program{sample_vert, sample_frag, "sample_program"sv};
    }

    auto create_frame_buffer(std::uint32_t width, std::uint32_t height, ufps::Sampler &sampler, ufps::TextureManager &texture_manager) -> ufps::FrameBuffer
    {
        const auto fb_texture_data = ufps::TextureData{
            .width = width,
            .height = height,
            .format = ufps::TextureFormat::RGB16F,
            .data = std::nullopt,
        };

        auto fb_texture = ufps::Texture{fb_texture_data, "fb_texture", sampler};
        const auto fb_texture_index = texture_manager.add(std::move(fb_texture));

        const auto depth_texture_data = ufps::TextureData{
            .width = width,
            .height = height,
            .format = ufps::TextureFormat::DEPTH24,
            .data = std::nullopt,
        };

        auto depth_texture = ufps::Texture{depth_texture_data, "depth_texture", sampler};
        const auto depth_texture_index = texture_manager.add(std::move(depth_texture));

        return {texture_manager.textures({fb_texture_index}), texture_manager.texture(depth_texture_index), "main_frame_buffer"};
    }

}

namespace ufps
{
    Renderer::Renderer(std::uint32_t width, std::uint32_t height, ResourceLoader &resource_loader, TextureManager &texture_manager)
        : _dummy_vao{0u, [](auto e)
                     { ::glDeleteBuffers(1, &e); }},
          _command_buffer{},
          _camera_buffer{sizeof(CameraData), "camera_buffer"},
          _light_buffer{sizeof(LightData), "light_buffer"},
          _object_data_buffer{sizeof(ObjectData), "object_data_buffer"},
          _program{create_program(resource_loader)},
          _fb_sampler{FilterType::NEAREST, FilterType::LINEAR, "fb_sampler"},
          _fb{create_frame_buffer(width, height, _fb_sampler, texture_manager)}
    {

        ::glGenVertexArrays(1u, &_dummy_vao);
        ::glBindVertexArray(_dummy_vao);

        _program.use();
    }

    auto Renderer::render(const Scene &scene) -> void
    {
        _fb.bind();
        ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        _camera_buffer.write(scene.camera.data_view(), 0zu);

        const auto [vertex_buffer_handle, index_buffer_handle] = scene.mesh_manager.native_handle();
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));
        ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_handle);

        const auto command_count = _command_buffer.build(scene);

        ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _command_buffer.native_handle());

        const auto object_data = scene.entities |
                                 std::views::transform([&scene](const auto &e)
                                                       { 
                                                        const auto index = scene.material_manager.index(e.material_key);
                                                         return ObjectData{
                                                            .model = e.transform, 
                                                            .material_id_index = index,
                                                             .padding={},}; }) |
                                 std::ranges::to<std::vector>();

        resize_gpu_buffer(object_data, _object_data_buffer, "object_data_buffer");

        _object_data_buffer.write(std::as_bytes(std::span{object_data.data(), object_data.size()}), 0zu);
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _object_data_buffer.native_handle());

        scene.material_manager.sync();
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, scene.material_manager.native_handle());

        _light_buffer.write(std::as_bytes(std::span<const LightData>{&scene.lights, 1zu}), 0zu);
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _light_buffer.native_handle());

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, scene.texture_manager.native_handle());

        ::glMultiDrawElementsIndirect(GL_TRIANGLES,
                                      GL_UNSIGNED_INT,
                                      reinterpret_cast<const void *>(_command_buffer.offset_bytes()),
                                      command_count,
                                      0);

        _command_buffer.advance();
        _camera_buffer.advance();
        _light_buffer.advance();
        _object_data_buffer.advance();
        scene.material_manager.advance();

        _fb.unbind();

        ::glBlitNamedFramebuffer(
            _fb.native_handle(),
            0u,
            0u,
            0u,
            _fb.width(),
            _fb.height(),
            0u,
            0u,
            _fb.width(),
            _fb.height(),
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST);
    }
}
