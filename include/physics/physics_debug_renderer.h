#pragma once

#include <string_view>
#include <vector>

#include "physics/jolt.h"

namespace ufps
{
    class PhysicsDebugRenderer : public ::JPH::DebugRendererSimple
    {
    public:
        auto DrawLine(::JPH::RVec3Arg from, ::JPH::RVec3Arg to, ::JPH::ColorArg color) -> void override;
        auto DrawTriangle(::JPH::RVec3Arg v1, ::JPH::RVec3Arg v2, ::JPH::RVec3Arg v3, ::JPH::ColorArg color, ECastShadow) -> void override;
        auto DrawText3D(::JPH::RVec3Arg, const std::string_view &, ::JPH::ColorArg, float) -> void override;
    };
}