#include "graphics/debug_ui.h"

#include <windows.h>

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include "window.h"

namespace ufps
{
    DebugUI::DebugUI(const Window &window)
        : _window{window} _click{},
          _selected_entity{}
    {
        IMGUI_CHECKVERSION();
        ::ImGui::CreateContext();
        auto &io = ::ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        ::ShowCursor();
        io.MouseDrawCursor = io.WantCaptureMouse;

        ::ImGui::StyleColorsDark();

        ::ImGui_ImplWin32_InitForOpenGL(window.native_handle());
        ::ImGui_ImplOpenGL3_Init();
    }

    DebugUI::~DebugUI()
    {
        ::ImGui_ImplOpenGL3_Shutdown();
        ::ImGui_ImplWin32_Shutdown();
        ::ImGui::DestroyContext();
    }

    auto DebugUI::new_frame() const -> void
    {
        ::ImGui_ImplWin32_NewFrame();
    }

}