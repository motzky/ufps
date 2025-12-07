#include "graphics/command_buffer.h"

#include <cstdint>
#include <ranges>
#include <string>

#include "graphics/multi_buffer.h"
#include "graphics/opengl.h"
#include "graphics/persistent_buffer.h"
#include "graphics/scene.h"
#include "log.h"
#include "utils/formatter.h"

namespace
{
    struct IndirectCommand
    {
        std::uint32_t count;
        std::uint32_t instanceCount;
        std::uint32_t first;
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
        const auto command = scene.entities |
                             std::views::transform(
                                 [](const auto &e)
                                 {
                                     return IndirectCommand{
                                         .count = e.mesh_view.count, .instanceCount = 1u, .first = e.mesh_view.offset, .baseInstance = 0u};
                                 }) |
                             std::ranges::to<std::vector>();

        const auto command_view = DataBufferView{reinterpret_cast<const std::byte *>(command.data()), command.size() * sizeof(IndirectCommand)};

        if (command_view.size_bytes() > _command_buffer.original_size())
        {
            auto new_size = _command_buffer.original_size() * 2zu;
            while (new_size < command_view.size_bytes())
            {
                new_size *= 2zu;
            }

            log::info("growing command buffer {} -> {}", _command_buffer.original_size(), new_size);

            // opengl barrier in case gpu using previous frame
            ::glFinish();
            _command_buffer = MultiBuffer<PersistentBuffer>{new_size, "mesh_data"};
        }

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

    auto CommandBuffer::to_string() const -> std::string
    {
        return std::format("command buffer {} size", _command_buffer.original_size());
    }
}