#include "graphics/renderer.h"

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

    auto create_frame_buffer(std::uint32_t width, std::uint32_t height, ufps::Sampler &sampler, ufps::TextureManager &texture_manager, std::uint32_t &fb_texture_index) -> ufps::FrameBuffer
    {
        const auto fb_texture_data = ufps::TextureData{
            .width = width,
            .height = height,
            .format = ufps::TextureFormat::RGB16F,
            .data = std::nullopt,
        };

        auto fb_texture = ufps::Texture{fb_texture_data, "fb_texture", sampler};
        fb_texture_index = texture_manager.add(std::move(fb_texture));

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
    Renderer::Renderer(std::uint32_t width, std::uint32_t height, ResourceLoader &resource_loader, TextureManager &texture_manager, MeshManager &mesh_manager)
        : _dummy_vao{0u, [](auto e)
                     { ::glDeleteVertexArrays(1, &e); }},
          _command_buffer{},
          _post_processing_command_buffer{},
          _post_process_sprite{
              .name = "post_process_sprite",
              .mesh_view = mesh_manager.load(sprite()),
              .transform = {},
              .material_key = {0u}},
          _camera_buffer{sizeof(CameraData), "camera_buffer"},
          _light_buffer{sizeof(LightData), "light_buffer"},
          _object_data_buffer{sizeof(ObjectData), "object_data_buffer"},
          _gbuffer_program{create_program(resource_loader, "sample_program"sv, "shaders/simple.vert"sv, "simple_vertex_shader"sv, "shaders/simple.frag"sv, "simple_fragement_shader"sv)},
          _light_pass_program{create_program(resource_loader, "light_pass_program"sv, "shaders/light_pass.vert"sv, "light_pass_vertex_shader"sv, "shaders/light_pass.frag"sv, "light_pass_fragement_shader"sv)},
          _fb_sampler{FilterType::NEAREST, FilterType::LINEAR, "fb_sampler"},
          _fb_texture_index{},
          _light_pass_fb_texture_index{},
          _fb{create_frame_buffer(width, height, _fb_sampler, texture_manager, _fb_texture_index)},
          _light_pass_fb{create_frame_buffer(width, height, _fb_sampler, texture_manager, _light_pass_fb_texture_index)}
    {

        ::glGenVertexArrays(1u, &_dummy_vao);
        ::glBindVertexArray(_dummy_vao);

        _post_processing_command_buffer.build(_post_process_sprite);
        _gbuffer_program.use();

        // ::glFrontFace(GL_CCW);
        // ::glCullFace(GL_BACK);
        // ::glEnable(GL_CULL_FACE);
    }

    auto Renderer::render(const Scene &scene) -> void
    {
        _fb.bind();
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

        _light_pass_fb.bind();
        ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        _light_pass_program.use();
        ::glProgramUniform1ui(_light_pass_program.native_handle(), 0u, _fb_texture_index);

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scene.texture_manager.native_handle());
        ::glMultiDrawElementsIndirect(GL_TRIANGLES,
                                      GL_UNSIGNED_INT,
                                      reinterpret_cast<const void *>(_post_processing_command_buffer.offset_bytes()),
                                      1,
                                      0);
        _light_pass_fb.unbind();

        ::glBlitNamedFramebuffer(
            _light_pass_fb.native_handle(),
            0u,
            0u,
            0u,
            _light_pass_fb.width(),
            _light_pass_fb.height(),
            0u,
            0u,
            _light_pass_fb.width(),
            _light_pass_fb.height(),
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST);

        _command_buffer.advance();
        _camera_buffer.advance();
        _light_buffer.advance();
        _object_data_buffer.advance();
        scene.material_manager.advance();

        // ::glBlitNamedFramebuffer(
        //     _fb.native_handle(),
        //     0u,
        //     0u,
        //     0u,
        //     _fb.width(),
        //     _fb.height(),
        //     0u,
        //     0u,
        //     _fb.width(),
        //     _fb.height(),
        //     GL_COLOR_BUFFER_BIT,
        //     GL_NEAREST);
    }
}
