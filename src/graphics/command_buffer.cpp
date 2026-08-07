#include "graphics/command_buffer.h"

#include <cstdint>
#include <ranges>
#include <string>
#include <string_view>

#include "core/render_entity_manager.h"
#include "core/scene.h"
#include "core/service_locator.h"
#include "graphics/multi_buffer.h"
#include "graphics/opengl.h"
#include "graphics/persistent_buffer.h"
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
    CommandBuffer::CommandBuffer(std::string_view name)
        : _command_buffer{1zu, name}
    {
    }

    auto CommandBuffer::build(const Scene &scene, EntityFilterMode filter_mode) -> std::uint32_t
    {
        auto rem = service<RenderEntityManager>();

        const auto command = scene.entities() |
                             std::views::transform(
                                 [](const auto &e)
                                 { return e.render_entities(); }) |
                             std::views::join |
                             std::views::filter([filter_mode, &rem](const auto &e)
                                                { 
                                                    switch(filter_mode)
                                                    {
                                                        using enum EntityFilterMode;
                                                        case ALL:
                                                            return true;
                                                        case OPAQUE:
                                                            return rem[e]->opacity() > 0.9999f;
                                                        case TRANSPARENT:
                                                            return rem[e]->opacity() < 1.f; 
                                                        default:
                                                            return true;
                                                    } }) |
                             std::views::transform(
                                 [&rem](auto e)
                                 {
                                     const auto entity = rem[e];

                                     return IndirectCommand{
                                         .count = entity->mesh_view().index_count,
                                         .instanceCount = 1u,
                                         .first_index = entity->mesh_view().index_offset,
                                         .base_vertex = static_cast<std::int32_t>(entity->mesh_view().vertex_offset),
                                         .baseInstance = 0u};
                                 }) |
                             std::ranges::to<std::vector>();

        const auto command_view = DataBufferView{reinterpret_cast<const std::byte *>(command.data()), command.size() * sizeof(IndirectCommand)};

        resize_gpu_buffer(command, _command_buffer);

        _command_buffer.write(command_view, 0u);

        return command.size();
    }

    auto CommandBuffer::build(const Entity &entity) -> std::uint32_t
    {
        auto rem = service<RenderEntityManager>();

        const auto command = entity.render_entities() |
                             std::views::transform(
                                 [&rem](auto e)
                                 {
                                     const auto entity = rem[e];
                                     return IndirectCommand{
                                         .count = entity->mesh_view().index_count,
                                         .instanceCount = 1u,
                                         .first_index = entity->mesh_view().index_offset,
                                         .base_vertex = static_cast<std::int32_t>(entity->mesh_view().vertex_offset),
                                         .baseInstance = 0u};
                                 }) |
                             std::ranges::to<std::vector>();

        const auto command_view = DataBufferView{reinterpret_cast<const std::byte *>(command.data()), command.size() * sizeof(IndirectCommand)};

        resize_gpu_buffer(std::vector<IndirectCommand>{command}, _command_buffer);

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

    auto CommandBuffer::name() const -> std::string_view
    {
        return _command_buffer.name();
    }
}