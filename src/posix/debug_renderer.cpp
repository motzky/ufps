#include "graphics/debug_renderer.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include "window.h"

namespace ufps
{
    DebugRenderer::DebugRenderer(const Window &window)
        : _window{window},
          _click{},
          _selected_entity{}
    {
        IMGUI_CHECKVERSION();
        ::ImGui::CreateContext();
        auto &io = ::ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.MouseDrawCursor = io.WantCaptureMouse;

        ::ImGui::StyleColorsDark();

        ::ImGui_ImplGlfw_InitForOpenGL(window.native_handle(), false);
        ::ImGui_ImplOpenGL3_Init();
    }

    DebugRenderer::~DebugRenderer()
    {
        ::ImGui_ImplOpenGL3_Shutdown();
        ::ImGui_ImplGlfw_Shutdown();
        ::ImGui::DestroyContext();
    }

    auto DebugRenderer::new_frame() const -> void
    {
        ::ImGui_ImplGlfw_NewFrame();
    }

}