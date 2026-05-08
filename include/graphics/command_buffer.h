#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "core/scene.h"
#include "graphics/multi_buffer.h"
#include "graphics/opengl.h"
#include "graphics/persistent_buffer.h"

namespace ufps
{
    enum class EntityFilterMode
    {
        ALL,
        OPAQUE,
        TRANSPARENT
    };

    class CommandBuffer
    {
    public:
        CommandBuffer(std::string_view name);

        auto build(const Scene &scene, EntityFilterMode filter_mode = EntityFilterMode::ALL) -> std::uint32_t;
        auto build(const Entity &entity) -> std::uint32_t;

        auto native_handle() const -> ::GLuint;

        auto advance() -> void;
        auto offset_bytes() const -> std::size_t;
        auto to_string() const -> std::string;
        auto name() const -> std::string_view;

    private:
        MultiBuffer<PersistentBuffer> _command_buffer;
    };
}