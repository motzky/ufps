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

namespace ufps
{
    class Renderer
    {
    public:
        Renderer(std::uint32_t width, std::uint32_t height, ResourceLoader &resource_loader, TextureManager &texture_manager, MeshManager &mesh_manager);

        auto render(const Scene &scene) -> void;

    private:
        AutoRelease<::GLuint> _dummy_vao;
        CommandBuffer _command_buffer;
        CommandBuffer _post_processing_command_buffer;
        Entity _post_process_sprite;
        MultiBuffer<PersistentBuffer> _camera_buffer;
        MultiBuffer<PersistentBuffer> _light_buffer;
        MultiBuffer<PersistentBuffer> _object_data_buffer;
        Program _gbuffer_program;
        Program _light_pass_program;
        Sampler _fb_sampler;
        std::uint32_t _fb_texture_index;
        std::uint32_t _light_pass_fb_texture_index;
        FrameBuffer _fb;
        FrameBuffer _light_pass_fb;
    };
}