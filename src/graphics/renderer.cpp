#include "graphics/renderer.h"

#include <cstdint>
#include <optional>
#include <random>
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

        const auto color_texture_bindless_handle_0 = texture_manager.texture(first_index)->bindless_handle();
        const auto color_texture_bindless_handle_1 = color_attachment_count > 1 ? texture_manager.texture(first_index + 1)->bindless_handle() : 0u;
        const auto color_texture_bindless_handle_2 = color_attachment_count > 2 ? texture_manager.texture(first_index + 2)->bindless_handle() : 0u;
        const auto color_texture_bindless_handle_3 = color_attachment_count > 3 ? texture_manager.texture(first_index + 3)->bindless_handle() : 0u;
        const auto color_texture_bindless_handle_4 = color_attachment_count > 4 ? texture_manager.texture(first_index + 4)->bindless_handle() : 0u;
        const auto color_texture_bindless_handle_5 = color_attachment_count > 5 ? texture_manager.texture(first_index + 5)->bindless_handle() : 0u;
        const auto color_texture_bindless_handle_6 = color_attachment_count > 6 ? texture_manager.texture(first_index + 6)->bindless_handle() : 0u;

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
            .color_texture_bindless_handle_0 = color_texture_bindless_handle_0,
            .color_texture_bindless_handle_1 = color_texture_bindless_handle_1,
            .color_texture_bindless_handle_2 = color_texture_bindless_handle_2,
            .color_texture_bindless_handle_3 = color_texture_bindless_handle_3,
            .color_texture_bindless_handle_4 = color_texture_bindless_handle_4,
            .color_texture_bindless_handle_5 = color_texture_bindless_handle_5,
            .color_texture_bindless_handle_6 = color_texture_bindless_handle_6,
            .depth_texture_bindless_handle = texture_manager.texture(depth_texture_index)->bindless_handle(),
        };
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

    auto create_sprite(ufps::MeshManager &mesh_manager, ufps::TextureManager & /*texture_manager*/) -> ufps::Entity
    {
        const auto mesh_data = std::vector{sprite()};
        const auto mesh_views = mesh_manager.load("sprite", mesh_data);

        return {
            "post_process_sprite",
            {{
                mesh_views.front(),
                0u,
                0u,
                0u,
                0u,
                0u,
                0u,
                // texture_manager.texture_index("textures/default_BaseColor.dds"),
                // texture_manager.texture_index("textures/default_Normal.dds"),
                // texture_manager.texture_index("textures/default_Metallic.dds"),
                // texture_manager.texture_index("textures/default_Roughness.dds"),
                // texture_manager.texture_index("textures/default_AO.dds"),
                // texture_manager.texture_index("textures/default_Emissive.dds"),
                false,
                1.f,
                0.f,
                mesh_manager,
            }},
            {}};
    }

    auto create_ssao_noise_texture(ufps::TextureManager &texture_manager, const ufps::Sampler &sampler) -> std::uint64_t
    {
        auto generator = std::mt19937{std::random_device{}()};
        auto distribution = std::uniform_real_distribution<float>{-1.f, 1.f};

        auto ssao_noise_data = ufps::DataBuffer{};

        for (auto i = 0u; i < 16u; ++i)
        {
            const auto x = distribution(generator);
            const auto y = distribution(generator);
            const auto z = 0.f;

            ssao_noise_data.push_back(static_cast<std::byte>((x * .5f + .5f) * 255.f));
            ssao_noise_data.push_back(static_cast<std::byte>((y * .5f + .5f) * 255.f));
            ssao_noise_data.push_back(static_cast<std::byte>((z * .5f + .5f) * 255.f));
        }

        const auto ssao_noise_texture_data = ufps::TextureData{
            .width = 4,
            .height = 4,
            .format = ufps::TextureFormat::RGB,
            .data = ssao_noise_data,
            .is_compressed = false,
        };

        auto tex = ufps::Texture{
            ssao_noise_texture_data,
            "ssao_noise_texture",
            sampler};

        const auto tex_index = texture_manager.add(std::move(tex));
        return texture_manager.texture(tex_index)->bindless_handle();
    }
}

namespace ufps
{
    Renderer::Renderer(const Window &window, ResourceLoader &resource_loader, TextureManager &texture_manager, MeshManager &mesh_manager)
        : _window{window},
          _dummy_vao{0u, [](auto e)
                     { ::glDeleteVertexArrays(1, &e); }},
          _command_buffer{"gbuffer_command_buffer"},
          _forward_transparancy_command_buffer{"forward_transparancy_command_buffer"},
          _post_processing_command_buffer{"post_processing_command_buffer"},
          _post_process_sprite{create_sprite(mesh_manager, texture_manager)},
          _camera_buffer{sizeof(CameraData), "camera_buffer"},                                                                                                                                                                      //
          _light_buffer{sizeof(LightData), "light_buffer"},                                                                                                                                                                         //
          _object_data_buffer{sizeof(ObjectData), "object_data_buffer"},                                                                                                                                                            //
          _transparent_object_data_buffer{sizeof(ObjectData), "transparent_object_data_buffer"},                                                                                                                                    //
          _luminance_histogram_buffer{sizeof(std::uint32_t) * 256, "luminance_histogram_buffer"},                                                                                                                                   //
          _average_luminance_buffer{sizeof(float) * 1, "average_luminance_buffer"},                                                                                                                                                 //
          _ssao_samples_buffer{sizeof(Vector4) * 64, "ssao_samples_buffer"},                                                                                                                                                        //
          _gbuffer_program{create_program(resource_loader, "gbuffer_program"sv, "shaders/gbuffer.vert"sv, "gbuffer_vertex_shader"sv, "shaders/gbuffer.frag"sv, "gbuffer_fragement_shader"sv)},                                      //
          _light_pass_program{create_program(resource_loader, "light_pass_program"sv, "shaders/light_pass.vert"sv, "light_pass_vertex_shader"sv, "shaders/light_pass.frag"sv, "light_pass_fragment_shader"sv)},                     //
          _forward_transparancy_program{create_program(resource_loader, "transparancy_program"sv, "shaders/transparancy.vert"sv, "transparancy_vertex_shader"sv, "shaders/transparancy.frag"sv, "transparancy_fragment_shader"sv)}, //
          _tone_map_program{create_program(resource_loader, "tone_map_program"sv, "shaders/tone_map.vert"sv, "tone_map_vertex_shader"sv, "shaders/tone_map.frag"sv, "tone_map_fragment_shader"sv)},                                 //
          _luminance_program{create_program(resource_loader, "luminance_histogram_program"sv, "shaders/luminance_histogram.comp"sv, "luminance_history_compute")},
          _average_luminance_program{create_program(resource_loader, "average_luminance_program"sv, "shaders/average_luminance.comp"sv, "average_luminance_compute")},
          _ssao_program{create_program(resource_loader, "ssao_program"sv, "shaders/ssao.vert"sv, "ssao_vertex_shader"sv, "shaders/ssao.frag"sv, "ssao_fragement_shader"sv)},                                                                                                 //
          _ssao_blur_program{create_program(resource_loader, "ssao_blur_program"sv, "shaders/ssao.vert"sv, "ssao_blur_vertex_shader"sv, "shaders/ssao_blur.frag"sv, "ssao_blur_fragement_shader"sv)},                                                                        //
          _chromatic_abberation_program{create_program(resource_loader, "chromatic_abberation_program"sv, "shaders/chromatic_abberation.vert"sv, "chromatic_abberation_vertex_shader"sv, "shaders/chromatic_abberation.frag"sv, "chromatic_abberation_fragement_shader"sv)}, //
          _bloom_downsample_program{create_program(resource_loader, "bloom_downsample_program"sv, "shaders/bloom_downsample.vert"sv, "bloom_downsample_vertex_shader"sv, "shaders/bloom_downsample.frag"sv, "bloom_downsample_fragement_shader"sv)},                         //
          _bloom_upsample_program{create_program(resource_loader, "bloom_upsample_program"sv, "shaders/bloom_upsample.vert"sv, "bloom_upsample_vertex_shader"sv, "shaders/bloom_upsample.frag"sv, "bloom_upsample_fragement_shader"sv)},                                     //
          _bloom_mix_program{create_program(resource_loader, "bloom_mix_program"sv, "shaders/bloom_mix.vert"sv, "bloom_mix_vertex_shader"sv, "shaders/bloom_mix.frag"sv, "bloom_mix_fragement_shader"sv)},                                                                   //
          _ssao_noise_sampler{FilterType::NEAREST, FilterType::NEAREST, WrapMode::REPEAT, WrapMode::REPEAT, "ssao_noise_sampler"},                                                                                                                                           //
          _ssao_noise_texture_bindless_handle{create_ssao_noise_texture(texture_manager, _ssao_noise_sampler)},                                                                                                                                                              //
          _fb_sampler{FilterType::LINEAR, FilterType::LINEAR, WrapMode::CLAMP_TO_EDGE, WrapMode::CLAMP_TO_EDGE, "fb_sampler"},                                                                                                                                               //
          _gbuffer_rt{create_render_target(7u, window.width(), window.height(), _fb_sampler, texture_manager, "gbuffer")},                                                                                                                                                   //
          _light_pass_rt{create_render_target(1u, window.width(), window.height(), _fb_sampler, texture_manager, "light_pass")},                                                                                                                                             //
          _forward_transparancy_rt{create_render_target(1u, window.width(), window.height(), _fb_sampler, texture_manager, "forward_transparancy")},                                                                                                                         //
          _tone_map_rt{create_render_target(1u, window.width(), window.height(), _fb_sampler, texture_manager, "tone_map")},
          _ssao_rt{create_render_target(1u, window.width() / 2u, window.height() / 2u, _fb_sampler, texture_manager, "ssao", TextureFormat::RG16F)},
          _ssao_blur_rt{create_render_target(1u, window.width() / 2u, window.height() / 2u, _fb_sampler, texture_manager, "ssao", TextureFormat::RG16F)},
          _chromatic_abberation_rt{create_render_target(1u, window.width(), window.height(), _fb_sampler, texture_manager, "chromatic_abberation")},
          _bloom_mips{},
          _bloom_rt{create_render_target(1u, window.width(), window.height(), _fb_sampler, texture_manager, "bloom")},
          _final_fb{}
    {

        ::glGenVertexArrays(1u, &_dummy_vao);
        ::glBindVertexArray(_dummy_vao);

        auto generator = std::mt19937{std::random_device{}()};
        auto distribution = std::uniform_real_distribution<float>{0.f, 1.f};

        auto ssao_samples = std::vector<Vector4>{};
        for (auto u = 0u; u < 64u; ++u)
        {
            auto sample = Vector3{
                distribution(generator) * 2.f - 1.f,
                distribution(generator) * 2.f - 1.f,
                distribution(generator)};
            sample = Vector3::normalize(sample);
            sample *= distribution(generator);

            auto scale = static_cast<float>(u) / 64.f;
            scale = std::lerp(.1f, 1.f, scale * scale);
            sample *= scale;

            ssao_samples.push_back(Vector4{sample, 0.f});
        }

        _ssao_samples_buffer.write(std::as_bytes(std::span{ssao_samples.data(), ssao_samples.size()}), 0u);

        static constexpr auto bloom_mip_count = 5u;
        for (auto i = 0u; i < bloom_mip_count; ++i)
        {
            const auto scale = std::pow(0.5, static_cast<float>((i + 1u) * 0.5f));
            _bloom_mips.push_back(
                create_render_target(
                    1u,
                    window.width() * scale,
                    window.height() * scale,
                    _fb_sampler, texture_manager,
                    std::format("bloom_mip_{}", i)));
        }

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

        execute_forward_transparancy_pass(scene);

        execute_bloom_pass(scene);

        execute_luminance_histogram_pass(scene);

        execute_luminance_average_pass(scene);

        execute_ssao_pass(scene);

        execute_tone_mapping_pass(scene);

        execute_chromatic_abberation_pass(scene);

        _final_fb = &_chromatic_abberation_rt.fb;

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

        const auto command_count = _command_buffer.build(scene, EntityFilterMode::OPAQUE);

        ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _command_buffer.native_handle());

        auto object_data = std::vector<ObjectData>{};

        for (const auto &entity : scene.entities())
        {
            object_data.append_range(
                entity.render_entities() |
                std::views::filter([](const auto &e)
                                   { return e.opacity() > 0.9999f; }) |
                std::views::transform(
                    [&entity](const auto &e)
                    { return ObjectData{
                          .model = entity.transform(),
                          .albedo_texture_bindless_handle = e.albedo_texture_bindless_handle(),
                          .normal_texture_bindless_handle = e.normal_texture_bindless_handle(),
                          .specular_texture_bindless_handle = e.specular_texture_bindless_handle(),
                          .roughness_texture_bindless_handle = e.roughness_texture_bindless_handle(),
                          .ao_texture_bindless_handle = e.ao_texture_bindless_handle(),
                          .emissive_texture_bindless_handle = e.emissive_texture_bindless_handle(),
                          .opacity = e.opacity(),
                          .emissive_strength = e.emissive_intensity() * entity.emissive_strength(),
                          .normal_compressed = e.normal_compressed() ? 1u : 0u,
                          .pad{},
                      }; }));
        }

        resize_gpu_buffer(object_data, _object_data_buffer);

        _object_data_buffer.write(std::as_bytes(std::span{object_data.data(), object_data.size()}), 0zu);
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, _object_data_buffer.native_handle(), _object_data_buffer.frame_offset_bytes(), _object_data_buffer.size());

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
            writer.write(lights.lights.data());
        }

        _light_pass_program.set_uniforms(_gbuffer_rt.color_texture_bindless_handle_0,
                                         _gbuffer_rt.color_texture_bindless_handle_1,
                                         _gbuffer_rt.color_texture_bindless_handle_2,
                                         _gbuffer_rt.color_texture_bindless_handle_3,
                                         _gbuffer_rt.color_texture_bindless_handle_4,
                                         _gbuffer_rt.color_texture_bindless_handle_5,
                                         _gbuffer_rt.color_texture_bindless_handle_6);

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, _light_buffer.native_handle(), _light_buffer.frame_offset_bytes(), _light_buffer.size());
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));
        ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _post_processing_command_buffer.native_handle());
        ::glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            reinterpret_cast<const void *>(_post_processing_command_buffer.offset_bytes()),
            1u,
            0);
    }

    auto Renderer::execute_forward_transparancy_pass(Scene &scene) -> void
    {
        _forward_transparancy_rt.fb.bind();

        ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ::glBlitNamedFramebuffer(
            _gbuffer_rt.fb.native_handle(),
            _forward_transparancy_rt.fb.native_handle(),
            0u,
            0u,
            _gbuffer_rt.fb.width(),
            _gbuffer_rt.fb.height(),
            0u,
            0u,
            _forward_transparancy_rt.fb.width(),
            _forward_transparancy_rt.fb.height(),
            GL_DEPTH_BUFFER_BIT,
            GL_NEAREST);

        ::glBlitNamedFramebuffer(
            _light_pass_rt.fb.native_handle(),
            _forward_transparancy_rt.fb.native_handle(),
            0u,
            0u,
            _light_pass_rt.fb.width(),
            _light_pass_rt.fb.height(),
            0u,
            0u,
            _forward_transparancy_rt.fb.width(),
            _forward_transparancy_rt.fb.height(),
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST);

        ::glDepthMask(GL_FALSE);
        ::glEnable(GL_BLEND);
        ::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        [[maybe_unused]] const auto auto_bind = AutoBind(_forward_transparancy_program);

        const auto [vertex_buffer_handle, index_buffer_handle] = scene.mesh_manager().native_handle();
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, _light_buffer.native_handle(), _light_buffer.frame_offset_bytes(), _light_buffer.size());
        ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_handle);

        const auto command_count = _forward_transparancy_command_buffer.build(scene, EntityFilterMode::TRANSPARENT);

        ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _forward_transparancy_command_buffer.native_handle());

        auto object_data = std::vector<ObjectData>{};

        for (const auto &entity : scene.entities())
        {
            object_data.append_range(
                entity.render_entities() |
                std::views::filter([](const auto &e)
                                   { return e.opacity() < 1.f; }) |
                std::views::transform(
                    [&entity](const auto &e)
                    { return ObjectData{
                          .model = entity.transform(),
                          .albedo_texture_bindless_handle = e.albedo_texture_bindless_handle(),
                          .normal_texture_bindless_handle = e.normal_texture_bindless_handle(),
                          .specular_texture_bindless_handle = e.specular_texture_bindless_handle(),
                          .roughness_texture_bindless_handle = e.roughness_texture_bindless_handle(),
                          .ao_texture_bindless_handle = e.ao_texture_bindless_handle(),
                          .emissive_texture_bindless_handle = e.emissive_texture_bindless_handle(),
                          .opacity = e.opacity(),
                          .emissive_strength = entity.emissive_strength(),
                          .normal_compressed = e.normal_compressed() ? 1u : 0u,
                          .pad{},
                      }; }));
        }

        resize_gpu_buffer(object_data, _transparent_object_data_buffer);

        _transparent_object_data_buffer.write(std::as_bytes(std::span{object_data.data(), object_data.size()}), 0zu);
        ::glBindBufferRange(
            GL_SHADER_STORAGE_BUFFER,
            2,
            _transparent_object_data_buffer.native_handle(),
            _transparent_object_data_buffer.frame_offset_bytes(),
            _transparent_object_data_buffer.size());

        ::glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            reinterpret_cast<const void *>(_forward_transparancy_command_buffer.offset_bytes()),
            command_count,
            0);

        ::glDisable(GL_BLEND);
        ::glDepthMask(GL_TRUE);
    }

    auto Renderer::execute_bloom_pass([[maybe_unused]] Scene &scene) -> void
    {
        auto src_width = _forward_transparancy_rt.fb.width();
        auto src_height = _forward_transparancy_rt.fb.height();
        auto src_handle = _forward_transparancy_rt.color_texture_bindless_handle_0;

        {
            [[maybe_unused]] const auto auto_bind = AutoBind{_bloom_downsample_program};
            for (const auto &mip : _bloom_mips)
            {
                mip.fb.bind();
                ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                ::glViewport(0, 0, mip.fb.width(), mip.fb.height());

                _bloom_downsample_program.set_uniforms(src_handle, std::make_tuple(src_width, src_height));

                const auto [vertex_buffer_handle, index_buffer_handle] = scene.mesh_manager().native_handle();
                ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
                ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _post_processing_command_buffer.native_handle());
                ::glMultiDrawElementsIndirect(
                    GL_TRIANGLES,
                    GL_UNSIGNED_INT,
                    reinterpret_cast<const void *>(_post_processing_command_buffer.offset_bytes()),
                    1u,
                    0);

                src_width = mip.fb.width();
                src_height = mip.fb.height();
                src_handle = mip.color_texture_bindless_handle_0;
            }
        }

        {
            [[maybe_unused]] const auto auto_bind = AutoBind{_bloom_upsample_program};
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);

            for (const auto &mip : std::views::reverse(_bloom_mips) | std::views::drop(1))
            {
                mip.fb.bind();
                ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                ::glViewport(0, 0, mip.fb.width(), mip.fb.height());

                _bloom_upsample_program.set_uniforms(src_handle, _bloom_filter_radius);

                const auto [vertex_buffer_handle, index_buffer_handle] = scene.mesh_manager().native_handle();
                ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
                ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _post_processing_command_buffer.native_handle());
                ::glMultiDrawElementsIndirect(
                    GL_TRIANGLES,
                    GL_UNSIGNED_INT,
                    reinterpret_cast<const void *>(_post_processing_command_buffer.offset_bytes()),
                    1u,
                    0);

                src_width = mip.fb.width();
                src_height = mip.fb.height();
                src_handle = mip.color_texture_bindless_handle_0;
            }

            glDisable(GL_BLEND);
        }

        ::glViewport(0, 0, _forward_transparancy_rt.fb.width(), _forward_transparancy_rt.fb.height());

        {
            [[maybe_unused]] const auto auto_bind = AutoBind{_bloom_mix_program};

            const auto &mip = _bloom_mips.front();

            _bloom_rt.fb.bind();
            ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            _bloom_mix_program.set_uniforms(
                mip.color_texture_bindless_handle_0,
                _forward_transparancy_rt.color_texture_bindless_handle_0,
                _bloom_filter_radius,
                _bloom_mix_amount);

            const auto [vertex_buffer_handle, index_buffer_handle] = scene.mesh_manager().native_handle();
            ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
            ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _post_processing_command_buffer.native_handle());
            ::glMultiDrawElementsIndirect(
                GL_TRIANGLES,
                GL_UNSIGNED_INT,
                reinterpret_cast<const void *>(_post_processing_command_buffer.offset_bytes()),
                1u,
                0);
        }
    }

    auto Renderer::execute_luminance_histogram_pass(Scene &scene) -> void
    {
        [[maybe_unused]] const auto auto_bind = AutoBind{_luminance_program};

        const auto zero = ::GLuint{0};
        ::glClearNamedBufferData(_luminance_histogram_buffer.native_handle(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

        _luminance_program.set_uniforms(_bloom_rt.color_texture_bindless_handle_0, scene.exposure_options().min_log_luminance, 1.f / (scene.exposure_options().max_log_luminance - scene.exposure_options().min_log_luminance));

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _luminance_histogram_buffer.native_handle());

        ::glDispatchCompute(
            static_cast<std::uint32_t>(_bloom_rt.fb.width() + 15 / 16),
            static_cast<std::uint32_t>(_bloom_rt.fb.height() + 15 / 16),
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
                                                static_cast<float>(_bloom_rt.fb.width() * _bloom_rt.fb.height()));

        ::glDispatchCompute(256, 1, 1);
        ::glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    }

    auto Renderer::execute_ssao_pass(Scene &scene) -> void
    {
        if (!scene.ssao_options().enabled)
        {
            ::glClearColor(1.f, 0.f, 0.f, 1.f);
            _ssao_blur_rt.fb.bind();
            ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            ::glClearColor(0.f, 0.f, 0.f, 1.f);

            return;
        }

        ::glViewport(0, 0, _ssao_rt.fb.width(), _ssao_rt.fb.height());
        const auto [vertex_buffer_handle, index_buffer_handle] = scene.mesh_manager().native_handle();

        {
            _ssao_rt.fb.bind();
            ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            [[maybe_unused]] const auto auto_bind = AutoBind{_ssao_program};

            _ssao_program.set_uniforms(_gbuffer_rt.color_texture_bindless_handle_1,
                                       _gbuffer_rt.color_texture_bindless_handle_2,
                                       _gbuffer_rt.color_texture_bindless_handle_5,
                                       _gbuffer_rt.color_texture_bindless_handle_6,
                                       static_cast<float>(_gbuffer_rt.fb.width()),
                                       static_cast<float>(_gbuffer_rt.fb.height()),
                                       scene.ssao_options().sample_count,
                                       scene.ssao_options().radius,
                                       scene.ssao_options().bias,
                                       scene.ssao_options().power,
                                       _ssao_noise_texture_bindless_handle);

            ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
            ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));
            ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _ssao_samples_buffer.native_handle());

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

            _ssao_blur_program.set_uniforms(_ssao_rt.color_texture_bindless_handle_0,
                                            _gbuffer_rt.depth_texture_bindless_handle,
                                            static_cast<float>(_ssao_rt.fb.width()),
                                            static_cast<float>(_ssao_rt.fb.height()));

            ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);

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

        _tone_map_program.set_uniforms(_bloom_rt.color_texture_bindless_handle_0,
                                       scene.tone_map_options().max_brightness,
                                       scene.tone_map_options().contrast,
                                       scene.tone_map_options().linear_section_start,
                                       scene.tone_map_options().linear_section_length,
                                       scene.tone_map_options().black_tightness,
                                       scene.tone_map_options().pedestal,
                                       scene.tone_map_options().gamma,
                                       _ssao_blur_rt.color_texture_bindless_handle_0,
                                       _gbuffer_rt.color_texture_bindless_handle_2,
                                       scene.fog_options().color,
                                       scene.fog_options().density);

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _average_luminance_buffer.native_handle());
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));
        ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _post_processing_command_buffer.native_handle());

        ::glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            reinterpret_cast<const void *>(_post_processing_command_buffer.offset_bytes()),
            1u,
            0);
    }

    auto Renderer::execute_chromatic_abberation_pass(Scene &scene) -> void
    {
        static const auto start = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

        const auto [vertex_buffer_handle, index_buffer_handle] = scene.mesh_manager().native_handle();

        _chromatic_abberation_rt.fb.bind();
        ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        [[maybe_unused]] const auto auto_bind = AutoBind{_chromatic_abberation_program};

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scene.texture_manager().native_handle());
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));

        _chromatic_abberation_program.set_uniforms(_tone_map_rt.color_texture_bindless_handle_0,
                                                   scene.chromatic_abberation_options().red_offset,
                                                   scene.chromatic_abberation_options().green_offset,
                                                   scene.chromatic_abberation_options().blue_offset,
                                                   scene.chromatic_abberation_options().strength,
                                                   scene.vignette_options().color,
                                                   scene.vignette_options().strength,
                                                   scene.vignette_options().feather,
                                                   scene.film_grain_options().strength,
                                                   static_cast<float>(elapsed.count()));

        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _post_processing_command_buffer.native_handle());

        ::glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            reinterpret_cast<const void *>(_post_processing_command_buffer.offset_bytes()),
            1u,
            0);
    }
}
