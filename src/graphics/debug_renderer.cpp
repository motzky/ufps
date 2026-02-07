#include "graphics/debug_renderer.h"

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
    auto DebugRenderer::post_render(Scene &scene) -> void
    {
        Renderer::post_render(scene);

        if (!_enabled)
        {
            return;
        }

        auto &io = ::ImGui::GetIO();

        ::ImGui_ImplOpenGL3_NewFrame();
        new_frame();
        ::ImGui::NewFrame();

        ::ImGuizmo::BeginFrame();
        ::ImGuizmo::SetOrthographic(false);
        ::ImGuizmo::Enable(true);
        ::ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

        ::ImGui::DockSpaceOverViewport(0, ::ImGui::GetMainViewport(), ::ImGuiDockNodeFlags_PassthruCentralNode);

        ::ImGui::Begin("scene");

        ::ImGui::LabelText("FPS", "%0.1f", io.Framerate);

        for (auto &entity : scene.entities)
        {
            ::ImGui::CollapsingHeader(entity.name.c_str());

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
            float amb_color[3]{};
            std::memcpy(amb_color, &scene.lights.ambient, sizeof(amb_color));

            if (::ImGui::ColorPicker3("ambient light color", amb_color))
            {
                std::memcpy(&scene.lights.ambient, amb_color, sizeof(amb_color));
            }

            float pos[3] = {scene.lights.light.position.x, scene.lights.light.position.y, scene.lights.light.position.z};

            if (::ImGui::SliderFloat3("position", pos, -100.f, 100.f))
            {
                scene.lights.light.position = {pos[0], pos[1], pos[2]};
            }

            float color[3]{};
            std::memcpy(color, &scene.lights.light.color, sizeof(color));

            if (::ImGui::ColorPicker3("light color", color))
            {
                std::memcpy(&scene.lights.light.color, color, sizeof(color));
            }

            ::ImGui::SliderFloat("power", &scene.lights.light.specular_poewr, 0.f, 128.f);

            float att[3] = {scene.lights.light.constant_attenuation, scene.lights.light.linear_attenuation, scene.lights.light.quadratic_attenuation};

            if (::ImGui::SliderFloat3("attenuation", att, 0.f, 2.f))
            {
                scene.lights.light.constant_attenuation = att[0];
                scene.lights.light.linear_attenuation = att[1];
                scene.lights.light.quadratic_attenuation = att[2];
            }

            if (!_selected_entity)
            {
                auto transform = Matrix4{scene.lights.light.position};

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
                scene.lights.light.position = new_transform.position;
            }
        }

        ::ImGui::End();

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

        ::ImGui::Begin("render_targets");

        const auto aspect_ratio = static_cast<float>(_window.width()) / static_cast<float>(_window.height());
        for (auto i = 0u; i < _gbuffer_rt.color_attachment_count; ++i)
        {
            const auto tex = scene.texture_manager.texture(_gbuffer_rt.first_color_attachment_index + i);
            ::ImGui::Image(tex->native_handle(), ::ImVec2(100.f * aspect_ratio, 100.f), ::ImVec2(0.f, 1.f), ::ImVec2(1.f, 0.f));
            ::ImGui::SameLine();
        }

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

    auto DebugRenderer::add_mouse_event(const MouseButtonEvent &evt) -> void
    {
        auto &io = ::ImGui::GetIO();

        io.AddMouseButtonEvent(0, evt.state() == MouseButtonState::DOWN);

        if (!io.WantCaptureMouse)
        {
            _click = evt;
        }
    }

    auto DebugRenderer::set_enabled(bool enabled) -> void
    {
        _enabled = enabled;
    }

}