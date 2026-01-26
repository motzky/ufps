#pragma once

#include "core/scene.h"
#include "events/mouse_button_event.h"
#include "window.h"

namespace ufps
{
    class DebugRenderer
    {
    public:
        DebugRenderer(const Window &window);
        ~DebugRenderer();

        auto render(Scene &scene) -> void;

        auto add_mouse_event(const MouseButtonEvent &evt) -> void;

    private:
        const Window &_window;
        std::optional<MouseButtonEvent> _click;
        const Entity *_selected_entity;

        auto new_frame() const -> void;
    };
}