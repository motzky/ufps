#pragma once

#include "concurrency/concurrent_queue.h"
#include "graphics/color.h"
#include "graphics/line_data.h"
#include "math/vector3.h"

namespace ufps
{
    enum class DebugLayerType
    {
        DEFAULT,
        DEBUG,
    };

    class DebugLayer
    {
    public:
        auto push_line(
            const Vector3 &start,
            const Vector3 &end,
            const Color &color,
            DebugLayerType type = DebugLayerType::DEFAULT) -> void;

        auto yield_lines(DebugLayerType type) -> std::queue<LineData>;

    private:
        ConcurrentQueue<LineData> _default_lines_queue;
        ConcurrentQueue<LineData> _debug_lines_queue;
    };

}