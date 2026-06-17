#include "physics/physics_debug_renderer.h"

#include <ranges>
#include <string_view>
#include <vector>

#include "physics/jolt.h"

namespace ufps
{
    auto PhysicsDebugRenderer::DrawLine(::JPH::RVec3Arg from, ::JPH::RVec3Arg to, ::JPH::ColorArg color) -> void
    {
        _lines.push_back({to_native(from), to_native(color)});
        _lines.push_back({to_native(to), to_native(color)});
    }

    auto PhysicsDebugRenderer::DrawTriangle(::JPH::RVec3Arg v1, ::JPH::RVec3Arg v2, ::JPH::RVec3Arg v3, ::JPH::ColorArg color, ECastShadow) -> void
    {
        DrawLine(v1, v2, color);
        DrawLine(v1, v3, color);
        DrawLine(v2, v3, color);
    }

    auto PhysicsDebugRenderer::DrawText3D(::JPH::RVec3Arg, const std::string_view &, ::JPH::ColorArg, float) -> void
    {
    }

    auto PhysicsDebugRenderer::yield_lines() -> std::vector<LineData>
    {
        auto copy = std::vector<LineData>{};
        std::ranges::swap(copy, _lines);
        return copy;
    }

}
