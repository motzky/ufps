#pragma once

#include <cstdint>

#include "core/scene.h"
#include "graphics/command_buffer.h"
#include "graphics/frame_buffer.h"
#include "graphics/mesh_manager.h"
#include "graphics/multi_buffer.h"
#include "graphics/opengl.h"
#include "graphics/persistent_buffer.h"
#include "graphics/program.h"
#include "graphics/sampler.h"
#include "graphics/texture_manager.h"
#include "resources/resource_loader.h"
#include "utils/auto_release.h"
#include "window.h"

namespace ufps
{
    struct RenderTarget
    {
        FrameBuffer fb;
        std::uint64_t color_texture_bindless_handle_0;
        std::uint64_t color_texture_bindless_handle_1;
        std::uint64_t color_texture_bindless_handle_2;
        std::uint64_t color_texture_bindless_handle_3;
        std::uint64_t color_texture_bindless_handle_4;
        std::uint64_t color_texture_bindless_handle_5;
        std::uint64_t color_texture_bindless_handle_6;
        std::uint64_t depth_texture_bindless_handle;
    };

    class Renderer
    {
    public:
        Renderer(const Window &window, ResourceLoader &resource_loader, TextureManager &texture_manager, MeshManager &mesh_manager);
        virtual ~Renderer() = default;

        auto render(Scene &scene) -> void;

    protected:
        static auto create_program(
            ufps::ResourceLoader &resource_loader,
            std::string_view program_name,
            std::string_view vertex_path,
            std::string_view vertex_name,
            std::string_view fragment_path,
            std::string_view fragment_name) -> ufps::Program;

        static auto create_program(
            ufps::ResourceLoader &resource_loader,
            std::string_view program_name,
            std::string_view compute_path,
            std::string_view compute_name) -> ufps::Program;

        virtual auto post_render(Scene &scene) -> void;

        const Window &_window;
        AutoRelease<::GLuint> _dummy_vao;
        CommandBuffer _command_buffer;
        CommandBuffer _forward_transparancy_command_buffer;
        CommandBuffer _post_processing_command_buffer;
        Entity _post_process_sprite;
        MultiBuffer<PersistentBuffer> _camera_buffer;
        MultiBuffer<PersistentBuffer> _light_buffer;
        MultiBuffer<PersistentBuffer> _object_data_buffer;
        MultiBuffer<PersistentBuffer> _transparent_object_data_buffer;
        Buffer _luminance_histogram_buffer;
        Buffer _average_luminance_buffer;
        Buffer _ssao_samples_buffer;
        Program _gbuffer_program;
        Program _light_pass_program;
        Program _forward_transparancy_program;
        Program _tone_map_program;
        Program _luminance_program;
        Program _average_luminance_program;
        Program _ssao_program;
        Program _ssao_blur_program;
        Program _chromatic_abberation_program;
        Program _bloom_downsample_program;
        Program _bloom_upsample_program;
        Program _bloom_mix_program;
        Sampler _ssao_noise_sampler;
        std::uint64_t _ssao_noise_texture_bindless_handle;
        Sampler _fb_sampler;
        RenderTarget _gbuffer_rt;
        RenderTarget _light_pass_rt;
        RenderTarget _forward_transparancy_rt;
        RenderTarget _tone_map_rt;
        RenderTarget _ssao_rt;
        RenderTarget _ssao_blur_rt;
        RenderTarget _chromatic_abberation_rt;
        std::vector<RenderTarget> _bloom_mips;
        RenderTarget _bloom_rt;
        FrameBuffer *_final_fb;
        float _bloom_filter_radius = 0.005f;
        float _bloom_mix_amount = 0.1f;
        float _bloom_threshold = 1.f;

    private:
        auto execute_gbuffer_pass(Scene &scene) -> void;
        auto execute_lighting_pass(Scene &scene) -> void;
        auto execute_bloom_pass(Scene &scene) -> void;
        auto execute_forward_transparancy_pass(Scene &scene) -> void;
        auto execute_luminance_histogram_pass(Scene &scene) -> void;
        auto execute_luminance_average_pass(Scene &scene) -> void;
        auto execute_ssao_pass(Scene &scene) -> void;
        auto execute_tone_mapping_pass(Scene &scene) -> void;
        auto execute_chromatic_abberation_pass(Scene &scene) -> void;
    };
}