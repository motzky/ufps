#include "graphics/debug_layer.h"

#include "concurrency/concurrent_queue.h"
#include "graphics/color.h"
#include "graphics/line_data.h"
#include "math/vector3.h"
#include "utils/exception.h"
#include "utils/formatter.h"

namespace ufps
{
    auto DebugLayer::push_line(const Vector3 &start, const Vector3 &end, const Color &color, DebugLayerType type) -> void
    {
        switch (type)
        {
            using enum DebugLayerType;

        case DEFAULT:
            _default_lines_queue.push({start, color});
            _default_lines_queue.push({end, color});
            break;
        case DEBUG:
            _debug_lines_queue.push({start, color});
            _debug_lines_queue.push({end, color});
            break;
        }
    }

    auto DebugLayer::push_cube(const Transform &transform, const Color &color, DebugLayerType type) -> void
    {
        switch (type)
        {
            using enum DebugLayerType;

        case DEFAULT:
            _default_cube_queue.push({transform, color});
            break;
        case DEBUG:
            _debug_cube_queue.push({transform, color});
            break;
        }
    }

    auto DebugLayer::push_aabb(const AABB &aabb, const Matrix4 &transform, const Color &color, DebugLayerType type) -> void
    {
        push_line(transform * Vector4{aabb.max.x, aabb.max.y, aabb.max.z, 1.f}, transform * Vector4{aabb.min.x, aabb.max.y, aabb.max.z, 1.f}, color, type);
        push_line(transform * Vector4{aabb.min.x, aabb.max.y, aabb.max.z, 1.f}, transform * Vector4{aabb.min.x, aabb.max.y, aabb.min.z, 1.f}, color, type);
        push_line(transform * Vector4{aabb.min.x, aabb.max.y, aabb.min.z, 1.f}, transform * Vector4{aabb.max.x, aabb.max.y, aabb.min.z, 1.f}, color, type);
        push_line(transform * Vector4{aabb.max.x, aabb.max.y, aabb.min.z, 1.f}, transform * Vector4{aabb.max.x, aabb.max.y, aabb.max.z, 1.f}, color, type); //

        push_line(transform * Vector4{aabb.max.x, aabb.max.y, aabb.max.z, 1.f}, transform * Vector4{aabb.max.x, aabb.min.y, aabb.max.z, 1.f}, color, type);
        push_line(transform * Vector4{aabb.min.x, aabb.max.y, aabb.max.z, 1.f}, transform * Vector4{aabb.min.x, aabb.min.y, aabb.max.z, 1.f}, color, type);
        push_line(transform * Vector4{aabb.min.x, aabb.max.y, aabb.min.z, 1.f}, transform * Vector4{aabb.min.x, aabb.min.y, aabb.min.z, 1.f}, color, type);
        push_line(transform * Vector4{aabb.max.x, aabb.max.y, aabb.min.z, 1.f}, transform * Vector4{aabb.max.x, aabb.min.y, aabb.min.z, 1.f}, color, type); //

        push_line(transform * Vector4{aabb.max.x, aabb.min.y, aabb.max.z, 1.f}, transform * Vector4{aabb.min.x, aabb.min.y, aabb.max.z, 1.f}, color, type);
        push_line(transform * Vector4{aabb.min.x, aabb.min.y, aabb.max.z, 1.f}, transform * Vector4{aabb.min.x, aabb.min.y, aabb.min.z, 1.f}, color, type);
        push_line(transform * Vector4{aabb.min.x, aabb.min.y, aabb.min.z, 1.f}, transform * Vector4{aabb.max.x, aabb.min.y, aabb.min.z, 1.f}, color, type);
        push_line(transform * Vector4{aabb.max.x, aabb.min.y, aabb.min.z, 1.f}, transform * Vector4{aabb.max.x, aabb.min.y, aabb.max.z, 1.f}, color, type);
    }

    auto DebugLayer::push_frustrum(const Camera &camera, const Color &color, DebugLayerType type) -> void
    {
        const auto &proj = camera.data().projection;
        const auto &view = camera.data().view;
        const auto &cam_pos = camera.data().position;

        const auto inv_vp = Matrix4::invert(proj * view);

        constexpr auto ndc_far_corners = std::array<Vector4, 4>{{{-1.f, 1.f, 1.f, 1.f},
                                                                 {1.f, 1.f, 1.f, 1.f},
                                                                 {1.f, -1.f, 1.f, 1.f},
                                                                 {-1.f, -1.f, 1.f, 1.f}}};

        static constexpr auto debug_distance = 3.0f;

        auto far_corners = std::array<Vector3, 4>{};
        for (auto i = 0zu; i < 4zu; ++i)
        {
            const auto world_h = inv_vp * ndc_far_corners[i];
            const auto world_pos = Vector3(world_h) / world_h.w;
            const auto ray_dir = Vector3::normalize(world_pos - cam_pos);

            far_corners[i] = cam_pos + ray_dir * debug_distance;
        }

        for (auto i = 0zu; i < 4zu; ++i)
        {
            push_line(cam_pos, far_corners[i], color, type);
        }

        for (auto i = 0zu; i < 4zu; ++i)
        {
            push_line(far_corners[i], far_corners[(i + 1) % 4], color, type);
        }
    }

    auto DebugLayer::yield_lines(DebugLayerType type) -> std::queue<LineData>
    {
        switch (type)
        {
            using enum DebugLayerType;

        case DEFAULT:
            return _default_lines_queue.yield();
        case DEBUG:
            return _debug_lines_queue.yield();
        }

        throw Exception("unknown type: {}", type);
    }

    auto DebugLayer::yield_cubes(DebugLayerType type) -> std::queue<CubeData>
    {
        switch (type)
        {
            using enum DebugLayerType;

        case DEFAULT:
            return _default_cube_queue.yield();
        case DEBUG:
            return _debug_cube_queue.yield();
        }

        throw Exception("unknown type: {}", type);
    }
}