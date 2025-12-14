#include "graphics/command_buffer.h"

#include <cstdint>
#include <ranges>
#include <string>

#include "graphics/multi_buffer.h"
#include "graphics/opengl.h"
#include "graphics/persistent_buffer.h"
#include "graphics/scene.h"
#include "graphics/utils.h"
#include "log.h"
#include "utils/formatter.h"

namespace
{
    struct IndirectCommand
    {
        std::uint32_t count;
        std::uint32_t instanceCount;
        std::uint32_t first_index;
        std::int32_t base_vertex;
        std::uint32_t baseInstance;
    };

}

namespace ufps
{
    CommandBuffer::CommandBuffer()
        : _command_buffer{1zu, "command_buffer"}
    {
    }

    auto CommandBuffer::build(const Scene &scene) -> std::uint32_t
    {
        auto base = 0;
        const auto command = scene.entities |
                             std::views::transform(
                                 [&base](const auto &e)
                                 {
                                     base += e.mesh_view.vertex_offset;
                                     return IndirectCommand{
                                         .count = e.mesh_view.index_count,
                                         .instanceCount = 1u,
                                         .first_index = e.mesh_view.index_offset,
                                         .base_vertex = base,
                                         .baseInstance = 0u};
                                 }) |
                             std::ranges::to<std::vector>();

        const auto command_view = DataBufferView{reinterpret_cast<const std::byte *>(command.data()), command.size() * sizeof(IndirectCommand)};

        resize_gpu_buffer(command, _command_buffer, "command_buffer");

        _command_buffer.write(command_view, 0u);

        return command.size();
    }

    auto CommandBuffer::native_handle() const -> ::GLuint
    {
        return _command_buffer.buffer().native_handle();
    }

    auto CommandBuffer::advance() -> void
    {
        _command_buffer.advance();
    }

    auto CommandBuffer::offset_bytes() const -> std::size_t
    {
        return _command_buffer.frame_offset_bytes();
    }

    auto CommandBuffer::to_string() const -> std::string
    {
        return std::format("command buffer {} size", _command_buffer.size());
    }
}