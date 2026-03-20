#pragma once

#include <optional>
#include <variant>
#include <vector>

#include "core/entity.h"
#include "core/scene.h"
#include "events/mouse_button_event.h"
#include "graphics/line_data.h"
#include "graphics/mesh_manager.h"
#include "graphics/point_light.h"
#include "graphics/renderer.h"
#include "graphics/texture_manager.h"
#include "resources/resource_loader.h"
#include "window.h"

namespace ufps
{
    class DebugRenderer : public Renderer
    {
    public:
        DebugRenderer(
            const Window &window,
            ResourceLoader &resource_loader,
            TextureManager &texture_manager,
            MeshManager &mesh_manager);
        ~DebugRenderer();

        auto add_mouse_event(const MouseButtonEvent &evt) -> void;

        auto set_enabled(bool enabled) -> void;

    protected:
        auto post_render(Scene &scene) -> void override;

    private:
        bool _enabled;
        std::optional<MouseButtonEvent> _click;
        std::variant<std::monostate, Entity *, PointLight *> _selected;
        std::vector<LineData> _debug_lines;
        MultiBuffer<PersistentBuffer> _debug_line_buffer;
        Program _debug_line_program;
        Program _debug_light_program;

        auto init_platform(const Window &window) const -> void;
        auto new_frame() const -> void;
    };
}