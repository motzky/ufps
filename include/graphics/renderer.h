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
        std::uint32_t first_color_attachment_index;
        std::uint32_t color_attachment_count;
        std::uint32_t depth_attachment_index;
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
        CommandBuffer _post_processing_command_buffer;
        Entity _post_process_sprite;
        MultiBuffer<PersistentBuffer> _camera_buffer;
        MultiBuffer<PersistentBuffer> _light_buffer;
        MultiBuffer<PersistentBuffer> _object_data_buffer;
        Buffer _luminance_histogram_buffer;
        Buffer _average_luminance_buffer;
        Program _gbuffer_program;
        Program _light_pass_program;
        Program _tone_map_program;
        Program _luminance_program;
        Program _average_luminance_program;
        Program _ssao_program;
        Sampler _fb_sampler;
        RenderTarget _gbuffer_rt;
        RenderTarget _light_pass_rt;
        RenderTarget _tone_map_rt;
        RenderTarget _ssao_rt;
        FrameBuffer *_final_fb;

    private:
        auto execute_gbuffer_pass(Scene &scene) -> void;
        auto execute_lighting_pass(Scene &scene) -> void;
        auto execute_luminance_histogram_pass(Scene &scene) -> void;
        auto execute_luminance_average_pass(Scene &scene) -> void;
        auto execute_ssao_pass(Scene &scene) -> void;
        auto execute_tone_mapping_pass(Scene &scene) -> void;
    };
}