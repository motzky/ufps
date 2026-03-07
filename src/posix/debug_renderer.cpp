#include "graphics/debug_renderer.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include "window.h"

namespace ufps
{

    auto DebugRenderer::init_platform(const Window &window) const -> void
    {
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