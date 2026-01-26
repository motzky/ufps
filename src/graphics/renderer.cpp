#include "graphics/renderer.h"

#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>

#include "core/camera.h"
#include "core/scene.h"
#include "graphics/command_buffer.h"
#include "graphics/mesh_manager.h"
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
#include "window.h"

using namespace std::literals;

namespace
{
    auto create_program(
        ufps::ResourceLoader &resource_loader,
        std::string_view program_name,
        std::string_view vertex_path,
        std::string_view vertex_name,
        std::string_view fragment_path,
        std::string_view fragment_name) -> ufps::Program
    {
        const auto sample_vert = ufps::Shader{resource_loader.load_string(vertex_path), ufps::ShaderType::VERTEX, vertex_name};
        const auto sample_frag = ufps::Shader{resource_loader.load_string(fragment_path), ufps::ShaderType::FRAGMENT, fragment_name};

        return ufps::Program{sample_vert, sample_frag, program_name};
    }

    auto create_render_taget(
        std::uint32_t color_attachment_count,
        std::uint32_t width,
        std::uint32_t height,
        ufps::Sampler &sampler,
        ufps::TextureManager &texture_manager,
        std::string_view name) -> ufps::RenderTarget
    {
        const auto color_attachment_texture_data = ufps::TextureData{
            .width = width,
            .height = height,
            .format = ufps::TextureFormat::RGB16F,
            .data = std::nullopt,
        };

        auto color_attachments =
            std::views::iota(0u, color_attachment_count) |
            std::views::transform(
                [&](auto index)
                {
                    return ufps::Texture{color_attachment_texture_data, std::format("{}_{}_texture", name, index), sampler};
                }) |
            std::ranges::to<std::vector>();

        const auto first_index = texture_manager.add(std::move(color_attachments));

        const auto depth_texture_data = ufps::TextureData{
            .width = width,
            .height = height,
            .format = ufps::TextureFormat::DEPTH24,
            .data = std::nullopt,
        };

        auto depth_texture = ufps::Texture{depth_texture_data, std::format("{}_depth_texture", name), sampler};
        const auto depth_texture_index = texture_manager.add(std::move(depth_texture));

        auto fb = ufps::FrameBuffer{
            texture_manager.textures(std::views::iota(first_index, first_index + color_attachment_count) | std::ranges::to<std::vector>()),
            texture_manager.texture(depth_texture_index),
            std::format("{}_frame_buffer", name)};

        return {
            .fb = std::move(fb),
            .first_color_attachment_index = first_index,
            .color_attachment_count = color_attachment_count,
            .depth_attachment_index = depth_texture_index};
    }

    auto sprite() -> ufps::MeshData
    {
        const ufps::Vector3 positions[] = {
            {-1.0f, 1.0f, 0.0f}, {-1.0f, -1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}};

        const ufps::UV uvs[] = {{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}};

        auto indices = std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3};
        return {.vertices = vertices(positions, positions, positions, positions, uvs),
                .indices = std::move(indices)};
    }
}

namespace ufps
{
    Renderer::Renderer(const Window &window, ResourceLoader &resource_loader, TextureManager &texture_manager, MeshManager &mesh_manager)
        : _window{window},
          _dummy_vao{0u, [](auto e)
                     { ::glDeleteVertexArrays(1, &e); }},
          _command_buffer{"gbuffer_command_buffer"},
          _post_processing_command_buffer{"post_processing_command_buffer"},
          _post_process_sprite{
              .name = "post_process_sprite",
              .mesh_view = mesh_manager.load(sprite()),
              .transform = {},
              .material_index = 0u},
          _camera_buffer{sizeof(CameraData), "camera_buffer"},
          _light_buffer{sizeof(LightData), "light_buffer"},
          _object_data_buffer{sizeof(ObjectData), "object_data_buffer"},
          _gbuffer_program{
              create_program(
                  resource_loader,
                  "gbuffer_program"sv,
                  "shaders/gbuffer.vert"sv,
                  "gbuffer_vertex_shader"sv,
                  "shaders/gbuffer.frag"sv,
                  "gbuffer_fragement_shader"sv)},
          _light_pass_program{
              create_program(
                  resource_loader,
                  "light_pass_program"sv,
                  "shaders/light_pass.vert"sv,
                  "light_pass_vertex_shader"sv,
                  "shaders/light_pass.frag"sv,
                  "light_pass_fragement_shader"sv)},
          _fb_sampler{FilterType::NEAREST, FilterType::LINEAR, "fb_sampler"},
          _gbuffer_rt{create_render_taget(4u, window.width(), window.height(), _fb_sampler, texture_manager, "gbuffer")},
          _light_pass_rt{create_render_taget(1u, window.width(), window.height(), _fb_sampler, texture_manager, "light_pass")}
    {

        ::glGenVertexArrays(1u, &_dummy_vao);
        ::glBindVertexArray(_dummy_vao);

        _post_processing_command_buffer.build(_post_process_sprite);

        // ::glFrontFace(GL_CCW);
        // ::glCullFace(GL_BACK);
        // ::glEnable(GL_CULL_FACE);
    }

    auto Renderer::render(Scene &scene) -> void
    {
        _gbuffer_rt.fb.bind();
        ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        _gbuffer_program.use();

        _camera_buffer.write(scene.camera.data_view(), 0zu);

        const auto [vertex_buffer_handle, index_buffer_handle] = scene.mesh_manager.native_handle();
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));
        ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_handle);

        const auto command_count = _command_buffer.build(scene);

        ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _command_buffer.native_handle());

        const auto object_data = scene.entities |
                                 std::views::transform([](const auto &e)
                                                       { return ObjectData{
                                                             .model = e.transform,
                                                             .material_id_index = e.material_index,
                                                             .padding = {}}; }) |
                                 std::ranges::to<std::vector>();

        resize_gpu_buffer(object_data, _object_data_buffer);

        _object_data_buffer.write(std::as_bytes(std::span{object_data.data(), object_data.size()}), 0zu);
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _object_data_buffer.native_handle());

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, scene.material_manager.native_handle());

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, scene.texture_manager.native_handle());

        ::glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            reinterpret_cast<const void *>(_command_buffer.offset_bytes()),
            command_count,
            0);

        _light_pass_rt.fb.bind();
        ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        _light_pass_program.use();

        ::glProgramUniform1ui(_light_pass_program.native_handle(), 0u, _gbuffer_rt.first_color_attachment_index + 0u);
        ::glProgramUniform1ui(_light_pass_program.native_handle(), 1u, _gbuffer_rt.first_color_attachment_index + 1u);
        ::glProgramUniform1ui(_light_pass_program.native_handle(), 2u, _gbuffer_rt.first_color_attachment_index + 2u);
        ::glProgramUniform1ui(_light_pass_program.native_handle(), 3u, _gbuffer_rt.first_color_attachment_index + 3u);

        _light_buffer.write(std::as_bytes(std::span<const LightData>{&scene.lights, 1zu}), 0zu);

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scene.texture_manager.native_handle());
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _light_buffer.native_handle());
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 3, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));
        ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _post_processing_command_buffer.native_handle());
        ::glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            reinterpret_cast<const void *>(_post_processing_command_buffer.offset_bytes()),
            1u,
            0);

        _command_buffer.advance();
        _camera_buffer.advance();
        _light_buffer.advance();
        _object_data_buffer.advance();

        post_render(scene);
    }

    auto Renderer::post_render(Scene &) -> void
    {
        _light_pass_rt.fb.unbind();

        ::glBlitNamedFramebuffer(
            _light_pass_rt.fb.native_handle(),
            0u,
            0u,
            0u,
            _light_pass_rt.fb.width(),
            _light_pass_rt.fb.height(),
            0u,
            0u,
            _light_pass_rt.fb.width(),
            _light_pass_rt.fb.height(),
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST);
    }
}
