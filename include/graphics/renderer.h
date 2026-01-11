#pragma once

#include "core/scene.h"
#include "graphics/command_buffer.h"
#include "graphics/multi_buffer.h"
#include "graphics/opengl.h"
#include "graphics/persistent_buffer.h"
#include "graphics/program.h"
#include "resources/resource_loader.h"
#include "utils/auto_release.h"

namespace ufps
{
    class Renderer
    {
    public:
        Renderer(ResourceLoader &resource_loader);

        auto render(const Scene &scene) -> void;

    private:
        AutoRelease<::GLuint> _dummy_vao;
        CommandBuffer _command_buffer;
        MultiBuffer<PersistentBuffer> _camera_buffer;
        MultiBuffer<PersistentBuffer> _light_buffer;
        MultiBuffer<PersistentBuffer> _object_data_buffer;
        Program _program;
    };
}