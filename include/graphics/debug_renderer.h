#pragma once

#include <optional>
#include <variant>
#include <vector>

#include "core/entity.h"
#include "core/entity_manager.h"
#include "core/scene.h"
#include "events/mouse_button_event.h"
#include "graphics/line_data.h"
#include "graphics/point_light.h"
#include "graphics/renderer.h"
#include "physics/physics_system.h"
#include "resources/resource_loader.h"
#include "window.h"

namespace ufps
{
    class DebugRenderer : public Renderer
    {
    public:
        DebugRenderer(const Window &window, ResourceLoader &resource_loader);
        ~DebugRenderer();

        auto add_mouse_event(const MouseButtonEvent &evt) -> void;

        auto set_enabled(bool enabled) -> void;

    protected:
        auto post_render(Scene &scene, const Camera &camera) -> void override;

    private:
        auto draw_post_processing_window(Scene &scene) -> void;
        auto draw_scene(Scene &scene, const Camera &camera) -> void;
        auto draw_logs() -> void;
        auto draw_bloom_mips() -> void;
        auto draw_render_targets() -> void;
        auto draw_inspector(Scene &scene) -> void;
        auto draw_gizmo(const Camera &camera) -> void;

        bool _enabled;
        bool _snap_enabled;
        std::optional<MouseButtonEvent> _click;
        std::variant<std::monostate, EntityHandle, LightHandle, RigidBodyHandle> _selected;
        std::vector<LineData> _debug_lines;
        MultiBuffer<PersistentBuffer> _debug_line_buffer;
        Program _debug_line_program;
        Program _debug_light_program;
        RenderEntityHandle _highlight_render_entity;
        EntityHandle _highlight_entity;
        RigidBodyHandle _highlight_rigid_body;

        auto init_platform(const Window &window) const -> void;
        auto new_frame() const -> void;
    };
}