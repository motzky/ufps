#include "graphics/shader.h"

#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "graphics/opengl.h"
#include "utils/auto_release.h"
#include "utils/exception.h"
#include "utils/formatter.h"

namespace
{
    auto to_native(ufps::ShaderType type) -> ::GLenum
    {
        switch (type)
        {
            using enum ufps::ShaderType;
        case VERTEX:
            return GL_VERTEX_SHADER;
        case FRAGMENT:
            return GL_FRAGMENT_SHADER;
        case COMPUTE:
            return GL_COMPUTE_SHADER;
        }
        throw ufps::Exception("unknown shader type: {}", std::to_underlying(type));
    }
}

namespace ufps
{
    Shader::Shader(std::string_view source, ShaderType type, std::string_view name)
        : _handle{}, _type(type)
    {
        _handle = AutoRelease<::GLuint>{::glCreateShader(to_native(type)), ::glDeleteShader};

        const ::GLchar *strings[] = {source.data()};
        const ::GLint lengths[] = {static_cast<::GLint>(source.length())};

        ::glShaderSource(_handle, 1, strings, lengths);
        ::glCompileShader(_handle);

        ::GLint result{};
        ::glGetShaderiv(_handle, GL_COMPILE_STATUS, &result);

        if (result != GL_TRUE)
        {
            char log[1024];
            ::glGetShaderInfoLog(_handle, sizeof(log), nullptr, log);

            ufps::ensure(result == GL_TRUE, "failed to compile shader {}\n{}", _type, log);
        }

        ::glObjectLabel(GL_SHADER, _handle, name.length(), name.data());
    }

    auto Shader::type() const -> ShaderType
    {
        return _type;
    }

    auto Shader::native_handle() const -> ::GLuint
    {
        return _handle;
    }

    auto to_string(ShaderType obj) -> std::string
    {
        switch (obj)
        {
            using enum ufps::ShaderType;
        case VERTEX:
            return "VERTEX";
        case FRAGMENT:
            return "FRAGMENT";
        case COMPUTE:
            return "COMPUTE";
        }

        throw ufps::Exception("unknown shader type: {}", std::to_underlying(obj));
    }

}
