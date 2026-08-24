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
}