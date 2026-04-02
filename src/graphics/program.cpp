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
        : _handle{},
          _is_bound{}
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

    Program::Program(const Shader &compute_shader, std::string_view name)
        : _handle{},
          _is_bound{}
    {
        expect(compute_shader.type() == ShaderType::COMPUTE, "shader must be a compute shader");
        _handle = {::glCreateProgram(), ::glDeleteProgram};
        ensure(_handle, "failed to create OpenGL program");

        ::glObjectLabel(GL_PROGRAM, _handle, name.length(), name.data());

        ::glAttachShader(_handle, compute_shader.native_handle());
        ::glLinkProgram(_handle);

        check_state(_handle, GL_LINK_STATUS, name, "failed to link program");

        ::glValidateProgram(_handle);
        check_state(_handle, GL_VALIDATE_STATUS, name, "failed to validate program");
    }

    auto Program::native_handle() const -> ::GLuint
    {
        return _handle;
    }

    auto Program::bind() -> void
    {
        expect(!_is_bound, "binding already bound program");
        ::glUseProgram(_handle);
        _is_bound = true;
    }

    auto Program::unbind() -> void
    {
        expect(_is_bound, "unbinding already unbound program");
        ::glUseProgram(0);
        _is_bound = false;
    }

    auto Program::set_uniform(std::size_t index, std::uint32_t value) const -> void
    {
        expect(_is_bound, "setting uniform on unbound program");
        ::glProgramUniform1ui(_handle, static_cast<std::uint32_t>(index), value);
    }

    auto Program::set_uniform(std::size_t index, float value) const -> void
    {
        expect(_is_bound, "setting uniform on unbound program");
        ::glProgramUniform1f(_handle, static_cast<std::uint32_t>(index), value);
    }

    auto Program::set_uniform(std::size_t index, const Matrix4 &value) const -> void
    {
        expect(_is_bound, "setting uniform on unbound program");
        ::glProgramUniformMatrix4fv(_handle, index, 1u, GL_FALSE, value.data().data());
    }

    auto Program::set_uniform(std::size_t index, const Color &value) const -> void
    {
        expect(_is_bound, "setting uniform on unbound program");
        ::glProgramUniform3f(_handle, index, value.r, value.g, value.b);
    }

}