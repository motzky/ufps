#include "graphics/debug_ui.h"

#include <cstring>
#include <string>

#include <imgui.h>

#include <ImGuizmo.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "events/mouse_button_event.h"
#include "graphics/scene.h"
#include "log.h"
#include "math/matrix4.h"
#include "window.h"

namespace ufps
{
    auto DebugUI::render([[maybe_unused]] Scene &scene) const -> void
    {
        auto &io = ::ImGui::GetIO();

        ::ImGui_ImplOpenGL3_NewFrame();
        new_frame();
        ::ImGui::NewFrame();

        ::ImGuizmo::BeginFrame();
        ::ImGuizmo::SetOrthographic(false);
        ::ImGuizmo::Enable(true);
        ::ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

        ::ImGui::LabelText("FPS", "%0.1f", io.Framerate);

        for (auto &entity : scene.entities)
        {
            auto &material = scene.material_manager[entity.material_key];

            if (::ImGui::CollapsingHeader(entity.name.c_str()))
            {
                float color[3]{};
                std::memcpy(color, &material.color, sizeof(color));
                const auto label = std::format("{} color", entity.name);

                if (::ImGui::ColorPicker3(label.c_str(), color))
                {
                    std::memcpy(&material.color, color, sizeof(color));
                }

                auto transform = Matrix4{entity.transform};

                const auto &camera_data = scene.camera.data();

                ::ImGuizmo::Manipulate(
                    camera_data.view.data().data(),
                    camera_data.projection.data().data(),
                    ::ImGuizmo::TRANSLATE | ::ImGuizmo::SCALE | ::ImGuizmo::ROTATE,
                    ::ImGuizmo::WORLD,
                    const_cast<float *>(transform.data().data()),
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr);

                entity.transform = Transform{transform};
            }
        }

        ::ImGui::Render();
        ::ImGui_ImplOpenGL3_RenderDrawData(::ImGui::GetDrawData());
    }

    auto DebugUI::add_mouse_event(const MouseButtonEvent &evt) const -> void
    {
        auto &io = ::ImGui::GetIO();

        io.AddMouseButtonEvent(0, evt.state() == MouseButtonState::DOWN);

        std::uint8_t buffer[4]{};

        ::glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        ::glReadBuffer(GL_BACK);

        ::glReadPixels(
            static_cast<::GLint>(evt.x()),
            static_cast<::GLint>(evt.y()),
            1,
            1,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            buffer);

        log::debug("r: {} g: {} b: {}", buffer[0], buffer[1], buffer[2]);
    }

}