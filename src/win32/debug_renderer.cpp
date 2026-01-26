#include "graphics/debug_renderer.h"

#include <windows.h>

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include "window.h"

namespace ufps
{
    DebugRenderer::DebugRenderer(
        const Window &window,
        ResourceLoader &resource_loader,
        TextureManager &texture_manager,
        MeshManager &mesh_manager)
        : Renderer{window, resource_loader, texture_manager, mesh_manager},
          _enabled{false},
          _click{},
          _selected_entity{}
    {
        IMGUI_CHECKVERSION();
        ::ImGui::CreateContext();
        auto &io = ::ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ::ShowCursor();
        io.MouseDrawCursor = io.WantCaptureMouse;

        ::ImGui::StyleColorsDark();

        ::ImGui_ImplWin32_InitForOpenGL(window.native_handle());
        ::ImGui_ImplOpenGL3_Init();
    }

    DebugRenderer::~DebugRenderer()
    {
        ::ImGui_ImplOpenGL3_Shutdown();
        ::ImGui_ImplWin32_Shutdown();
        ::ImGui::DestroyContext();
    }

    auto DebugRenderer::new_frame() const -> void
    {
        ::ImGui_ImplWin32_NewFrame();
    }

}