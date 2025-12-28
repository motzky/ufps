#include "graphics/sampler.h"

#include <optional>
#include <string>

#include "graphics/opengl.h"
#include "utils/auto_release.h"
#include "utils/ensure.h"
#include "utils/exception.h"
#include "utils/formatter.h"

namespace
{
    auto to_opengl(ufps::FilterType filter_type) -> ::GLenum
    {
        switch (filter_type)
        {
            using enum ufps::FilterType;
        case LINEAR_MIPMAP:
            return GL_LINEAR_MIPMAP_LINEAR;
        case LINEAR:
            return GL_LINEAR;
        case NEAREST:
            return GL_NEAREST;
        }

        throw ufps::Exception("unknown filter type: {}", filter_type);
    }
}

namespace ufps
{
    Sampler::Sampler(FilterType min_filter, FilterType mag_filter, const std::string &name, std::optional<float> ansisotropic_samples)
        : _handle{0u, [](auto sampler)
                  { ::glDeleteSamplers(1, &sampler); }},
          _name(name)
    {
        ::glCreateSamplers(1, &_handle);
        ::glObjectLabel(GL_SAMPLER, _handle, name.length(), name.data());

        ::glSamplerParamerteri(_handle, GL_TEXTURE_MIN_FILTER, to_opengl(min_filter));
        ::glSamplerParamerteri(_handle, GL_TEXTURE_MAG_FILTER, to_opengl(mag_filter));
        if (ansisotropic_samples)
        {
            expect(*ansisotropic_samples >= 1.f, "invalid samples: {}", *ansisotropic_samples);
            ::glSamplerParamerterf(_handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, *ansisotropic_samples);
        }
    }

    auto Sampler::native_handle() const -> ::GLuint
    {
        return _handle;
    }

    auto Sampler::name() const -> std::string
    {
        return _name;
    }

    auto to_string(FilterType filter_type) -> std::string
    {
        switch (filter_type)
        {
            using enum ufps::FilterType;
        case LINEAR_MIPMAP:
            return "LINEAR_MIPMAP_LINEAR";
        case LINEAR:
            return "LINEAR";
        case NEAREST:
            return "NEAREST";
        default:
            return "<unknown>";
        }
    }

}