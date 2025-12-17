#include "graphics/debug_ui.h"

#include <cstring>
#include <string>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include "events/mouse_button_event.h"
#include "graphics/scene.h"
#include "window.h"

namespace ufps
{
    DebugUI::DebugUI(const Window &window)
        : _window{window}
    {
        IMGUI_CHECKVERSION();
        ::ImGui::CreateContext();
        auto &io = ::ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.MouseDrawCursor = io.WantCaptureMouse;

        ::ImGui::StyleColorsDark();

        ::ImGui_ImplGlfw_InitForOpenGL(window.native_handle(), false);
        ::ImGui_ImplOpenGL3_Init();
    }

    DebugUI::~DebugUI()
    {
        ::ImGui_ImplOpenGL3_Shutdown();
        ::ImGui_ImplGlfw_Shutdown();
        ::ImGui::DestroyContext();
    }

    auto DebugUI::render([[maybe_unused]] Scene &scene) const -> void
    {
        auto &io = ::ImGui::GetIO();

        ::ImGui_ImplOpenGL3_NewFrame();
        ::ImGui_ImplGlfw_NewFrame();
        ::ImGui::NewFrame();

        ::ImGui::LabelText("FPS", "%0.1f", io.Framerate);

        for (const auto &entity : scene.entities)
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
            }
        }

        ::ImGui::Render();
        ::ImGui_ImplOpenGL3_RenderDrawData(::ImGui::GetDrawData());
    }

    auto DebugUI::add_mouse_event(const MouseButtonEvent &evt) const -> void
    {
        auto &io = ::ImGui::GetIO();

        io.AddMouseButtonEvent(0, evt.state() == MouseButtonState::DOWN);
    }

}