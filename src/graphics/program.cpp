#include "graphics/program.h"

#include <string>
#include <string_view>

#include "graphics/opengl.h"
#include "graphics/shader.h"
#include "utils/auto_release.h"
#include "utils/ensure.h"

namespace
{
    auto check_state(::GLuint handle, ::GLenum state, std::string_view name, std::string_view message) -> void
    {
        ::GLint result{};
        ::glGetProgramiv(handle, state, &result);

        if (result == GL_TRUE)
        {
            return;
        }

        char log[512];
        ::glGetProgramInfoLog(handle, sizeof(log), nullptr, log);

        throw ufps::Exception("{} '{}':\n{}", message, name, log);
    }

}

namespace ufps
{
    Program::Program(const Shader &vertex_shader, const Shader &fragment_shader, std::string_view name)
    {
        expect(vertex_shader.type() == ShaderType::VERTEX, "vertex_shader must be a vertex shader");
        expect(fragment_shader.type() == ShaderType::FRAGMENT, "fragment_shader must be a fragment shader");

        _handle = {::glCreateProgram(), ::glDeleteProgram};
        ensure(_handle, "failed to create OpenGL program");

        ::glObjectLabel(GL_PROGRAM, _handle, name.length(), name.data());

        ::glAttachShader(_handle, vertex_shader.native_handle());
        ::glAttachShader(_handle, fragment_shader.native_handle());
        ::glLinkProgram(_handle);

        check_state(_handle, GL_LINK_STATUS, name, "failed to link program");

        ::glValidateProgram(_handle);
        check_state(_handle, GL_VALIDATE_STATUS, name, "failed to validate program");
    }

    auto Program::use() -> void
    {
        ::glUseProgram(_handle);
    }
}