#pragma once

#include "concurrency/concurrent_queue.h"
#include "core/camera.h"
#include "graphics/color.h"
#include "graphics/line_data.h"
#include "math/aabb.h"
#include "math/matrix4.h"
#include "math/transform.h"
#include "math/vector3.h"

namespace ufps
{
    enum class DebugLayerType
    {
        DEFAULT,
        DEBUG,
    };

    struct CubeData
    {
        Transform transform;
        Color color;
    };

    class DebugLayer
    {
    public:
        auto push_line(
            const Vector3 &start,
            const Vector3 &end,
            const Color &color,
            DebugLayerType type = DebugLayerType::DEFAULT) -> void;

        auto push_cube(const Transform &tranform, const Color &color, DebugLayerType type = DebugLayerType::DEBUG) -> void;

        auto push_aabb(const AABB &aabb, const Matrix4 &transform, const Color &color, DebugLayerType type = DebugLayerType::DEBUG) -> void;
        auto push_frustrum(const Camera &camera, const Color &color, DebugLayerType type = DebugLayerType::DEBUG) -> void;

        auto yield_lines(DebugLayerType type) -> std::queue<LineData>;

        auto yield_cubes(DebugLayerType type) -> std::queue<CubeData>;

    private:
        ConcurrentQueue<LineData> _default_lines_queue;
        ConcurrentQueue<LineData> _debug_lines_queue;
        ConcurrentQueue<CubeData> _default_cube_queue;
        ConcurrentQueue<CubeData> _debug_cube_queue;
    };

}