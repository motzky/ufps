#include "physics/physics_debug_renderer.h"

#include <ranges>
#include <string_view>
#include <vector>

#include "core/service_locator.h"
#include "graphics/debug_layer.h"
#include "physics/jolt.h"

namespace ufps
{
    auto PhysicsDebugRenderer::DrawLine(::JPH::RVec3Arg from, ::JPH::RVec3Arg to, ::JPH::ColorArg color) -> void
    {
        service<DebugLayer>().push_line(to_native(from), to_native(to), to_native(color), DebugLayerType::DEBUG);
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

}
