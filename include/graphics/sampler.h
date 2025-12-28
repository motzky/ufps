#pragma once

#include <optional>
#include <string>

#include "opengl.h"
#include "utils/auto_release.h"

namespace ufps
{
    enum class FilterType
    {
        LINEAR_MIPMAP,
        LINEAR,
        NEAREST
    };

    class Sampler
    {
    public:
        Sampler(FilterType min_filter, FilterType mag_filter, const std::string &name, std::optional<float> ansisotropic_samples = std::nullopt);

        auto native_handle() const -> ::GLuint;

        auto name() const -> std::string;

    private:
        AutoRelease<::GLuint> _handle;
        std::string _name;
    };

    auto to_string(FilterType filter_type) -> std::string;
}