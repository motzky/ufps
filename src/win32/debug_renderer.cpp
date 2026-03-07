#include "graphics/debug_renderer.h"

#include <windows.h>

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include "window.h"

namespace ufps
{
    auto DebugRenderer::init_platform(const Window &window) const -> void
    {
        ::ShowCursor();
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