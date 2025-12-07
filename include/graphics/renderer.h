#pragma once

#include "graphics/command_buffer.h"
#include "graphics/opengl.h"
#include "graphics/program.h"
#include "graphics/scene.h"
#include "utils/auto_release.h"

namespace ufps
{
    class Renderer
    {
    public:
        Renderer();

        auto render(const Scene &scene) -> void;

    private:
        AutoRelease<::GLuint> _dummy_vao;
        CommandBuffer _command_buffer;
        Program _program;
    };
}