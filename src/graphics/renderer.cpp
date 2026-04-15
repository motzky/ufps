#include "graphics/renderer.h"

#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>

#include "core/camera.h"
#include "core/scene.h"
#include "graphics/buffer_writer.h"
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
    template <class T>
    struct AutoBind
    {
        AutoBind(T &obj)
            : obj{obj}
        {
            obj.bind();
        }
        ~AutoBind()
        {
            obj.unbind();
        }
        T &obj;
    };

    auto create_render_target(
        std::uint32_t color_attachment_count,
        std::uint32_t width,
        std::uint32_t height,
        ufps::Sampler &sampler,
        ufps::TextureManager &texture_manager,
        std::string_view name,
        ufps::TextureFormat format = ufps::TextureFormat::RGB16F) -> ufps::RenderTarget
    {
        const auto color_attachment_texture_data = ufps::TextureData{
            .width = width,
            .height = height,
            .format = format,
            .data = std::nullopt,
            .is_compressed = false,
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
            .is_compressed = false,
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

    auto create_sprite(ufps::MeshManager &mesh_manager) -> ufps::Entity
    {
        const auto mesh_data = std::vector{sprite()};
        const auto mesh_views = mesh_manager.load("sprite", mesh_data);

        return {"post_process_sprite", {{mesh_views.front(), 0u, mesh_manager}}, {}};
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
          _post_process_sprite{create_sprite(mesh_manager)},
          _camera_buffer{sizeof(CameraData), "camera_buffer"},                                                                                                                                                  //
          _light_buffer{sizeof(LightData), "light_buffer"},                                                                                                                                                     //
          _object_data_buffer{sizeof(ObjectData), "object_data_buffer"},                                                                                                                                        //
          _luminance_histogram_buffer{sizeof(std::uint32_t) * 256, "luminance_histogram_buffer"},                                                                                                               //
          _average_luminance_buffer{sizeof(float) * 1, "average_luminance_buffer"},                                                                                                                             //
          _gbuffer_program{create_program(resource_loader, "gbuffer_program"sv, "shaders/gbuffer.vert"sv, "gbuffer_vertex_shader"sv, "shaders/gbuffer.frag"sv, "gbuffer_fragement_shader"sv)},                  //
          _light_pass_program{create_program(resource_loader, "light_pass_program"sv, "shaders/light_pass.vert"sv, "light_pass_vertex_shader"sv, "shaders/light_pass.frag"sv, "light_pass_fragment_shader"sv)}, //
          _tone_map_program{create_program(resource_loader, "tone_map_program"sv, "shaders/tone_map.vert"sv, "tone_map_vertex_shader"sv, "shaders/tone_map.frag"sv, "tone_map_fragment_shader"sv)},             //
          _luminance_program{create_program(resource_loader, "luminance_histogram_program"sv, "shaders/luminance_histogram.comp"sv, "luminance_history_compute")},
          _average_luminance_program{create_program(resource_loader, "average_luminance_program"sv, "shaders/average_luminance.comp"sv, "average_luminance_compute")},
          _ssao_program{create_program(resource_loader, "ssao_program"sv, "shaders/ssao.vert"sv, "ssao_vertex_shader"sv, "shaders/ssao.frag"sv, "ssao_fragement_shader"sv)},                          //
          _ssao_blur_program{create_program(resource_loader, "ssao_blur_program"sv, "shaders/ssao.vert"sv, "ssao_blur_vertex_shader"sv, "shaders/ssao_blur.frag"sv, "ssao_blur_fragement_shader"sv)}, //
          _fb_sampler{FilterType::NEAREST, FilterType::LINEAR, "fb_sampler"},                                                                                                                         //
          _gbuffer_rt{create_render_target(7u, window.width(), window.height(), _fb_sampler, texture_manager, "gbuffer")},                                                                            //
          _light_pass_rt{create_render_target(1u, window.width(), window.height(), _fb_sampler, texture_manager, "light_pass")},                                                                      //
          _tone_map_rt{create_render_target(1u, window.width(), window.height(), _fb_sampler, texture_manager, "tone_map")},
          _ssao_rt{create_render_target(1u, window.width() / 2u, window.height() / 2u, _fb_sampler, texture_manager, "ssao", TextureFormat::RG16F)},
          _ssao_blur_rt{create_render_target(1u, window.width() / 2u, window.height() / 2u, _fb_sampler, texture_manager, "ssao", TextureFormat::RG16F)},
          _final_fb{}
    {

        ::glGenVertexArrays(1u, &_dummy_vao);
        ::glBindVertexArray(_dummy_vao);

        _post_processing_command_buffer.build(_post_process_sprite);

        // ::glFrontFace(GL_CCW);
        // ::glCullFace(GL_BACK);
        ::glEnable(GL_CULL_FACE);
    }

    auto Renderer::create_program(
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

    auto Renderer::create_program(
        ufps::ResourceLoader &resource_loader,
        std::string_view program_name,
        std::string_view compute_path,
        std::string_view compute_name) -> ufps::Program
    {
        const auto comp_shader = ufps::Shader{resource_loader.load_string(compute_path), ufps::ShaderType::COMPUTE, compute_name};

        return ufps::Program{comp_shader, program_name};
    }

    auto Renderer::render(Scene &scene) -> void
    {
        _camera_buffer.write(scene.camera().data_view(), 0zu);

        execute_gbuffer_pass(scene);

        execute_lighting_pass(scene);

        execute_luminance_histogram_pass(scene);

        execute_luminance_average_pass(scene);

        execute_ssao_pass(scene);

        execute_tone_mapping_pass(scene);

        _final_fb = &_tone_map_rt.fb;

        post_render(scene);

        _command_buffer.advance();
        _camera_buffer.advance();
        _light_buffer.advance();
        _object_data_buffer.advance();
    }

    auto Renderer::post_render(Scene &) -> void
    {
        _final_fb->unbind();

        ::glBlitNamedFramebuffer(
            _final_fb->native_handle(),
            0u,
            0u,
            0u,
            _final_fb->width(),
            _final_fb->height(),
            0u,
            0u,
            _final_fb->width(),
            _final_fb->height(),
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST);
    }

    auto Renderer::execute_gbuffer_pass(Scene &scene) -> void
    {
        _gbuffer_rt.fb.bind();
        ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        [[maybe_unused]] const auto auto_bind = AutoBind(_gbuffer_program);

        const auto [vertex_buffer_handle, index_buffer_handle] = scene.mesh_manager().native_handle();
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));
        ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_handle);

        const auto command_count = _command_buffer.build(scene);

        ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _command_buffer.native_handle());

        auto object_data = std::vector<ObjectData>{};

        for (const auto &entity : scene.entities())
        {
            object_data.append_range(
                entity.render_entities() |
                std::views::transform(
                    [&entity](const auto &e)
                    { return ObjectData{
                          .model = entity.transform(),
                          .material_id_index = e.material_index(),
                          .padding = {}}; }));
        }

        resize_gpu_buffer(object_data, _object_data_buffer);

        _object_data_buffer.write(std::as_bytes(std::span{object_data.data(), object_data.size()}), 0zu);
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _object_data_buffer.native_handle());

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, scene.material_manager().native_handle());

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, scene.texture_manager().native_handle());

        ::glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            reinterpret_cast<const void *>(_command_buffer.offset_bytes()),
            command_count,
            0);
    }

    auto Renderer::execute_lighting_pass(Scene &scene) -> void
    {
        _light_pass_rt.fb.bind();
        ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        [[maybe_unused]] const auto auto_bind = AutoBind{_light_pass_program};

        const auto [vertex_buffer_handle, index_buffer_handle] = scene.mesh_manager().native_handle();

        ::glEnable(GL_BLEND);
        ::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        {
            const auto &lights = scene.lights();
            const auto buffer_size_bytes = sizeof(lights.ambient) + sizeof(std::uint32_t) + sizeof(PointLight) * lights.lights.size();
            if (_light_buffer.size() < buffer_size_bytes)
            {
                _light_buffer = {buffer_size_bytes, _light_buffer.name()};
                ::glFinish();
            }

            auto writer = BufferWriter{_light_buffer};
            writer.write(lights.ambient);
            writer.write(static_cast<std::uint32_t>(lights.lights.size()));
            writer.write<PointLight>(lights.lights);
        }

        _light_pass_program.set_uniforms(_gbuffer_rt.first_color_attachment_index + 0u,
                                         _gbuffer_rt.first_color_attachment_index + 1u,
                                         _gbuffer_rt.first_color_attachment_index + 2u,
                                         _gbuffer_rt.first_color_attachment_index + 3u,
                                         _gbuffer_rt.first_color_attachment_index + 4u,
                                         _gbuffer_rt.first_color_attachment_index + 5u,
                                         _gbuffer_rt.first_color_attachment_index + 6u);

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scene.texture_manager().native_handle());
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _light_buffer.native_handle());
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 3, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));
        ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _post_processing_command_buffer.native_handle());
        ::glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            reinterpret_cast<const void *>(_post_processing_command_buffer.offset_bytes()),
            1u,
            0);

        ::glDisable(GL_BLEND);
    }

    auto Renderer::execute_luminance_histogram_pass(Scene &scene) -> void
    {
        [[maybe_unused]] const auto auto_bind = AutoBind{_luminance_program};

        const auto zero = ::GLuint{0};
        ::glClearNamedBufferData(_luminance_histogram_buffer.native_handle(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

        _luminance_program.set_uniforms(_light_pass_rt.first_color_attachment_index, scene.exposure_options().min_log_luminance, 1.f / (scene.exposure_options().max_log_luminance - scene.exposure_options().min_log_luminance));

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, scene.texture_manager().native_handle());
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _luminance_histogram_buffer.native_handle());

        ::glDispatchCompute(
            static_cast<std::uint32_t>(_light_pass_rt.fb.width() + 15 / 16),
            static_cast<std::uint32_t>(_light_pass_rt.fb.height() + 15 / 16),
            1);

        ::glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    }

    auto Renderer::execute_luminance_average_pass(Scene &scene) -> void
    {
        static auto delta_time = 1.f / 240.f;

        [[maybe_unused]] const auto auto_bind = AutoBind{_average_luminance_program};

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _luminance_histogram_buffer.native_handle());
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _average_luminance_buffer.native_handle());

        _average_luminance_program.set_uniforms(scene.exposure_options().min_log_luminance,
                                                scene.exposure_options().max_log_luminance - scene.exposure_options().min_log_luminance,
                                                std::clamp(1.f - std::exp(-delta_time * scene.exposure_options().tau), 0.f, 1.f),
                                                static_cast<float>(_light_pass_rt.fb.width() * _light_pass_rt.fb.height()));

        ::glDispatchCompute(256, 1, 1);
        ::glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    }

    auto Renderer::execute_ssao_pass(Scene &scene) -> void
    {
        ::glViewport(0, 0, _ssao_rt.fb.width(), _ssao_rt.fb.height());
        const auto [vertex_buffer_handle, index_buffer_handle] = scene.mesh_manager().native_handle();

        {
            _ssao_rt.fb.bind();
            ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            [[maybe_unused]] const auto auto_bind = AutoBind{_ssao_program};

            _ssao_program.set_uniforms(_gbuffer_rt.first_color_attachment_index + 1u,
                                       _gbuffer_rt.first_color_attachment_index + 2u,
                                       _gbuffer_rt.first_color_attachment_index + 5u,
                                       _gbuffer_rt.first_color_attachment_index + 6u,
                                       static_cast<float>(_ssao_rt.fb.width()),
                                       static_cast<float>(_ssao_rt.fb.height()),
                                       scene.ssao_options().sample_count,
                                       scene.ssao_options().radius,
                                       scene.ssao_options().bias);

            ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
            ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scene.texture_manager().native_handle());
            ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));

            ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _post_processing_command_buffer.native_handle());

            ::glMultiDrawElementsIndirect(
                GL_TRIANGLES,
                GL_UNSIGNED_INT,
                reinterpret_cast<const void *>(_post_processing_command_buffer.offset_bytes()),
                1u,
                0);
        }

        {
            _ssao_blur_rt.fb.bind();
            ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            [[maybe_unused]] const auto auto_bind = AutoBind{_ssao_blur_program};

            _ssao_blur_program.set_uniforms(_ssao_rt.first_color_attachment_index,
                                            _gbuffer_rt.depth_attachment_index,
                                            static_cast<float>(_ssao_rt.fb.width()),
                                            static_cast<float>(_ssao_rt.fb.height()));

            ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
            ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scene.texture_manager().native_handle());

            ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _post_processing_command_buffer.native_handle());

            ::glMultiDrawElementsIndirect(
                GL_TRIANGLES,
                GL_UNSIGNED_INT,
                reinterpret_cast<const void *>(_post_processing_command_buffer.offset_bytes()),
                1u,
                0);
        }

        ::glViewport(0, 0, _window.width(), _window.height());
    }

    auto Renderer::execute_tone_mapping_pass(Scene &scene) -> void
    {
        const auto [vertex_buffer_handle, index_buffer_handle] = scene.mesh_manager().native_handle();

        _tone_map_rt.fb.bind();
        ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        [[maybe_unused]] const auto auto_bind = AutoBind{_tone_map_program};

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scene.texture_manager().native_handle());
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));

        _tone_map_program.set_uniforms(_light_pass_rt.first_color_attachment_index + 0u,
                                       scene.tone_map_options().max_brightness,
                                       scene.tone_map_options().contrast,
                                       scene.tone_map_options().linear_section_start,
                                       scene.tone_map_options().linear_section_length,
                                       scene.tone_map_options().black_tightness,
                                       scene.tone_map_options().pedestal,
                                       scene.tone_map_options().gamma,
                                       _ssao_blur_rt.first_color_attachment_index);

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scene.texture_manager().native_handle());
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _average_luminance_buffer.native_handle());

        ::glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            reinterpret_cast<const void *>(_post_processing_command_buffer.offset_bytes()),
            1u,
            0);
    }
}
