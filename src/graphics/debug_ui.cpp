#include "graphics/debug_ui.h"

#include <cstring>
#include <string>

#include <imgui.h>

#include <ImGuizmo.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "core/scene.h"
#include "events/mouse_button_event.h"
#include "log.h"
#include "math/matrix4.h"
#include "math/ray.h"
#include "window.h"

namespace
{
    auto screen_ray(const ufps::MouseButtonEvent &evt, const ufps::Window &window, const ufps::Camera &camera) -> ufps::Ray
    {
        const auto x = 2.0f * evt.x() / window.width() - 1.f;
        const auto y = 1.f - 2.f * evt.y() / window.height();
        const auto ray_clip = ufps::Vector4{x, y, -1.f, 1.f};

        const auto inv_proj = ufps::Matrix4::invert(camera.data().projection);
        auto ray_eye = inv_proj * ray_clip;
        ray_eye = ufps::Vector4{ray_eye.x, ray_eye.y, -1.f, 0.f};

        const auto inv_view = ufps::Matrix4::invert(camera.data().view);
        const auto dir_world_space = ufps::Vector3::normalize(ufps::Vector3{inv_view * ray_eye});
        const auto origin_world_space = ufps::Vector3{inv_view[12], inv_view[13], inv_view[14]};

        return {origin_world_space, dir_world_space};
    }
}

namespace ufps
{
    auto DebugUI::render([[maybe_unused]] Scene &scene) -> void
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
            // TODO: make opening work reliably
            // const auto is_selected = &entity == _selected_entity;
            auto &material = scene.material_manager[entity.material_key];

            // const auto header_flags = is_selected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None;
            // if (::ImGui::CollapsingHeader(entity.name.c_str()), header_flags)
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

            if (&entity == _selected_entity)
            {
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

        if (::ImGui::CollapsingHeader("lights"))
        {
            float pos[3] = {scene.light.position.x, scene.light.position.y, scene.light.position.z};

            if (::ImGui::SliderFloat3("position", pos, 0.f, 2.f))
            {
                scene.light.position = {pos[0], pos[1], pos[2]};
            }

            float color[3]{};
            std::memcpy(color, &scene.light.color, sizeof(color));

            if (::ImGui::ColorPicker3("light color", color))
            {
                std::memcpy(&scene.light.color, color, sizeof(color));
            }

            float att[3] = {scene.light.constant_attenuation, scene.light.linear_attenuation, scene.light.quadratic_attenuation};

            if (::ImGui::SliderFloat3("attenuation", att, -100.f, 100.f))
            {
                scene.light.constant_attenuation = att[0];
                scene.light.linear_attenuation = att[1];
                scene.light.quadratic_attenuation = att[2];
            }

            if (!_selected_entity)
            {
                auto transform = Matrix4{scene.light.position};

                const auto &camera_data = scene.camera.data();

                ::ImGuizmo::Manipulate(
                    camera_data.view.data().data(),
                    camera_data.projection.data().data(),
                    ::ImGuizmo::TRANSLATE,
                    ::ImGuizmo::WORLD,
                    const_cast<float *>(transform.data().data()),
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr);

                const auto new_transform = Transform{transform};
                scene.light.position = new_transform.position;
            }
        }

        static auto follow = true;
        ::ImGui::Begin("Log");

        auto following_again = false;
        if (::ImGui::Checkbox("follow log", &follow))
        {
            log::debug("checkbox pressed");
            following_again = true;
        }

        ::ImGui::BeginChild("log output");

        for (const auto &pair : log::history)
        {
            switch (pair.first)
            {
                using enum log::Level;
            case DEBUG:
                ::ImGui::TextColored({0.f, .5f, 1.f, 1.f}, "%s\n", pair.second.c_str());
                break;
            case INFO:
                ::ImGui::TextColored({1.f, 1.f, 1.f, 1.f}, "%s\n", pair.second.c_str());
                break;
            case WARN:
                ::ImGui::TextColored({0.f, 1.f, 1.f, 1.f}, "%s\n", pair.second.c_str());
                break;
#ifndef WIN32
            case ERROR:
#else
            case ERR:
#endif
                ::ImGui::TextColored({1.f, 0.f, 0.f, 1.f}, "%s\n", pair.second.c_str());
                break;
            default:
                ::ImGui::TextColored({1.f, 1.f, 1.f, 1.f}, "%s\n", pair.second.c_str());
                break;
            }
        }

        if (!following_again && ::ImGui::GetScrollY() != ::ImGui::GetScrollMaxY())
        {
            follow = false;
        }

        if (follow)
        {
            ::ImGui::SetScrollHereY(1.f);
        }

        ::ImGui::EndChild();

        ::ImGui::End();

        ::ImGui::Render();
        ::ImGui_ImplOpenGL3_RenderDrawData(::ImGui::GetDrawData());

        if (_click)
        {
            const auto pick_ray = screen_ray(_click.value(), _window, scene.camera);
            const auto intersection = scene.intersect_ray(pick_ray);
            _selected_entity = intersection.transform([](const auto &e)
                                                      { return e.entity; })
                                   .value_or(nullptr);

            _click.reset();
        }
    }

    auto DebugUI::add_mouse_event(const MouseButtonEvent &evt) -> void
    {
        auto &io = ::ImGui::GetIO();

        io.AddMouseButtonEvent(0, evt.state() == MouseButtonState::DOWN);

        if (!io.WantCaptureMouse)
        {
            _click = evt;
        }
    }

}