#pragma once

#include <string>
#include <string_view>

#include "graphics/opengl.h"
#include "graphics/shader.h"
#include "utils/auto_release.h"

namespace ufps
{
    class Program
    {
    public:
        Program(const Shader &vertex_shader, const Shader &fragment_shader, std::string_view name);
        auto use() -> void;
        auto native_handle() const -> ::GLuint;

    private:
        AutoRelease<::GLuint> _handle;
    };
}