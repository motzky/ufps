#include "graphics/debug_renderer.h"

#include <algorithm>
#include <cstring>
#include <ranges>
#include <string>
#include <type_traits>

#include <imgui.h>

#include <ImGuizmo.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "core/scene.h"
#include "events/mouse_button_event.h"
#include "graphics/point_light.h"
#include "log.h"
#include "math/aabb.h"
#include "math/matrix4.h"
#include "math/ray.h"
#include "math/transform.h"
#include "window.h"

namespace
{
    static constexpr auto debugl_light_scale = 0.25f;

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

    auto draw_line(const ufps::Vector3 &start, const ufps::Vector3 &end, const ufps::Color &color, std::vector<ufps::LineData> &lines) -> void
    {
        lines.push_back({start, color});
        lines.push_back({end, color});
    }

    auto create_aabb_lines(const ufps::AABB &aabb, const ufps::Matrix4 &transform, const ufps::Color &color) -> std::vector<ufps::LineData>
    {
        auto lines = std::vector<ufps::LineData>{};
        lines.reserve(24);

        draw_line(transform * ufps::Vector4{aabb.max.x, aabb.max.y, aabb.max.z, 1.f}, transform * ufps::Vector4{aabb.min.x, aabb.max.y, aabb.max.z, 1.f}, color, lines);
        draw_line(transform * ufps::Vector4{aabb.min.x, aabb.max.y, aabb.max.z, 1.f}, transform * ufps::Vector4{aabb.min.x, aabb.max.y, aabb.min.z, 1.f}, color, lines);
        draw_line(transform * ufps::Vector4{aabb.min.x, aabb.max.y, aabb.min.z, 1.f}, transform * ufps::Vector4{aabb.max.x, aabb.max.y, aabb.min.z, 1.f}, color, lines);
        draw_line(transform * ufps::Vector4{aabb.max.x, aabb.max.y, aabb.min.z, 1.f}, transform * ufps::Vector4{aabb.max.x, aabb.max.y, aabb.max.z, 1.f}, color, lines); //

        draw_line(transform * ufps::Vector4{aabb.max.x, aabb.max.y, aabb.max.z, 1.f}, transform * ufps::Vector4{aabb.max.x, aabb.min.y, aabb.max.z, 1.f}, color, lines);
        draw_line(transform * ufps::Vector4{aabb.min.x, aabb.max.y, aabb.max.z, 1.f}, transform * ufps::Vector4{aabb.min.x, aabb.min.y, aabb.max.z, 1.f}, color, lines);
        draw_line(transform * ufps::Vector4{aabb.min.x, aabb.max.y, aabb.min.z, 1.f}, transform * ufps::Vector4{aabb.min.x, aabb.min.y, aabb.min.z, 1.f}, color, lines);
        draw_line(transform * ufps::Vector4{aabb.max.x, aabb.max.y, aabb.min.z, 1.f}, transform * ufps::Vector4{aabb.max.x, aabb.min.y, aabb.min.z, 1.f}, color, lines); //

        draw_line(transform * ufps::Vector4{aabb.max.x, aabb.min.y, aabb.max.z, 1.f}, transform * ufps::Vector4{aabb.min.x, aabb.min.y, aabb.max.z, 1.f}, color, lines);
        draw_line(transform * ufps::Vector4{aabb.min.x, aabb.min.y, aabb.max.z, 1.f}, transform * ufps::Vector4{aabb.min.x, aabb.min.y, aabb.min.z, 1.f}, color, lines);
        draw_line(transform * ufps::Vector4{aabb.min.x, aabb.min.y, aabb.min.z, 1.f}, transform * ufps::Vector4{aabb.max.x, aabb.min.y, aabb.min.z, 1.f}, color, lines);
        draw_line(transform * ufps::Vector4{aabb.max.x, aabb.min.y, aabb.min.z, 1.f}, transform * ufps::Vector4{aabb.max.x, aabb.min.y, aabb.max.z, 1.f}, color, lines);

        return lines;
    }
}

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
          _selected{std::monostate{}},
          _debug_lines{},
          _debug_line_buffer{sizeof(LineData) * 2u, "line_data_buffer"},
          _debug_line_program{create_program(resource_loader,
                                             "debug_lines_program",
                                             "shaders/line.vert",
                                             "line_vertex_shader",
                                             "shaders/line.frag",
                                             "line_fragment_shader")},
          _debug_light_program{create_program(resource_loader,
                                              "debug_light_program",
                                              "shaders/debug_light.vert",
                                              "debug_light_vertex_shader",
                                              "shaders/debug_light.frag",
                                              "debug_light_fragment_shader")}
    {
        IMGUI_CHECKVERSION();
        ::ImGui::CreateContext();
        auto &io = ::ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.MouseDrawCursor = io.WantCaptureMouse;

        ::ImGui::StyleColorsDark();

        init_platform(window);
    }

    auto DebugRenderer::post_render(Scene &scene) -> void
    {
        if (std::holds_alternative<Entity *>(_selected))
        {
            const auto *selected_entity = std::get<Entity *>(_selected);
            auto aabb_lines =
                selected_entity->render_entities() |
                std::views::transform([&](const auto &e)
                                      { return create_aabb_lines(e.aabb(), selected_entity->transform(), {0.4f, 0.4f, .4f}); }) |
                std::views::join;

            _debug_lines.append_range(aabb_lines);
            _debug_lines.append_range(create_aabb_lines(selected_entity->aabb(), selected_entity->transform(), {0.f, 1.f, 0.f}));
        }

        Renderer::post_render(scene);

        if (!_enabled)
        {
            return;
        }

        _light_pass_rt.fb.unbind();
        ::glBlitNamedFramebuffer(
            _gbuffer_rt.fb.native_handle(),
            0,
            0u,
            0u,
            _gbuffer_rt.fb.width(),
            _gbuffer_rt.fb.height(),
            0u,
            0u,
            _gbuffer_rt.fb.width(),
            _gbuffer_rt.fb.height(),
            GL_DEPTH_BUFFER_BIT,
            GL_NEAREST);

        _debug_light_program.use();

        const auto [vertex_buffer_handle, index_buffer_handle] = scene.mesh_manager().native_handle();
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));
        ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_handle);

        const auto cube_parts = scene.mesh_manager().mesh("cube");
        ensure(cube_parts.size() == 1u, "cube mesh should have exactly 1 part");
        const auto cube_indices_offset_bytes = cube_parts.front().index_offset * sizeof(std::uint32_t);

        for (const auto &light : scene.lights().lights)
        {
            const auto light_transform = Transform{light.position, {debugl_light_scale}, {}};
            const auto light_model = Matrix4{light_transform};

            const auto debug_light_aabb = ufps::AABB{
                .min = light_model * Vector4{-1.f, -1.f, -1.f, 1.f},
                .max = light_model * Vector4{1.f}};

            _debug_lines.append_range(create_aabb_lines(debug_light_aabb, {}, {1.f, 0.f, 0.f}));

            ::glProgramUniformMatrix4fv(_debug_light_program.native_handle(), 0u, 1u, GL_FALSE, light_model.data().data());
            ::glProgramUniform3f(
                _debug_light_program.native_handle(),
                1u,
                light.color.r,
                light.color.g,
                light.color.b);

            ::glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, reinterpret_cast<const void *>(cube_indices_offset_bytes));
        }

        auto debug_line_count = 0zu;

        if (!_debug_lines.empty())
        {
            _debug_line_program.use();
            debug_line_count = _debug_lines.size();

            resize_gpu_buffer(_debug_lines, _debug_line_buffer);
            _debug_line_buffer.write(std::as_bytes(std::span{_debug_lines.data(), _debug_lines.size()}), 0zu);
            ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _debug_line_buffer.native_handle());
            ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));

            ::glDrawArrays(GL_LINES, 0, _debug_lines.size());

            _debug_lines.clear();
            _debug_line_buffer.advance();
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
        ::ImGui::LabelText("Debug Line Count", "%zu", debug_line_count);

        if (::ImGui::Button("add light"))
        {
            scene.add(
                PointLight{
                    .position = {},
                    .color = {.r = 1.f, .g = 1.f, .b = 1.f},
                    .constant_attenuation = 1.f,
                    .linear_attenuation = 0.007f,
                    .quadratic_attenuation = 0.0002f,
                    .specular_poewr = 32.f});
            _selected = &scene.lights().lights.back();
        }

        {
            auto value = scene.tone_map_options().max_brightness;
            if (::ImGui::SliderFloat("Max Brightness", &value, 0.f, 100.f))
            {
                scene.tone_map_options().max_brightness = value;
            }

            value = scene.tone_map_options().contrast;
            if (::ImGui::SliderFloat("Contrast", &value, 0.f, 5.f))
            {
                scene.tone_map_options().contrast = value;
            }

            value = scene.tone_map_options().linear_section_start;
            if (::ImGui::SliderFloat("Linear Section Start", &value, 0.f, 1.f))
            {
                scene.tone_map_options().linear_section_start = value;
            }

            value = scene.tone_map_options().linear_section_length;
            if (::ImGui::SliderFloat("Linear Section Lenght", &value, 0.f, 1.f))
            {
                scene.tone_map_options().linear_section_length = value;
            }

            value = scene.tone_map_options().black_tightness;
            if (::ImGui::SliderFloat("Black tightness", &value, 0.f, 3.f))
            {
                scene.tone_map_options().black_tightness = value;
            }

            value = scene.tone_map_options().pedestal;
            if (::ImGui::SliderFloat("Pedestal", &value, 0.f, 1.f))
            {
                scene.tone_map_options().pedestal = value;
            }

            value = scene.tone_map_options().gamma;
            if (::ImGui::SliderFloat("Gamma", &value, 0.f, 5.f))
            {
                scene.tone_map_options().gamma = value;
            }
        }

        std::uint32_t histogram[256]{};
        ::glGetNamedBufferSubData(_luminance_histogram_buffer.native_handle(), 0, sizeof(histogram), histogram);
        const auto scaled_histogram = histogram |
                                      std::views::transform([](const auto e)
                                                            { return std::log2(static_cast<float>(e) + 1.f); }) |
                                      std::ranges::to<std::vector>();

        ::ImGui::PlotHistogram(
            "luminance",
            scaled_histogram.data(),
            256,
            0,
            nullptr,
            0.f,
            std::ranges::max(scaled_histogram),
            ::ImVec2(::ImGui::GetContentRegionAvail().x, 150.f));

        auto names = scene.mesh_manager().mesh_names();
        const auto mesh_names_cstr = names |
                                     std::views::transform([](const auto &e)
                                                           { return e.c_str(); }) |
                                     std::ranges::to<std::vector>();

        auto mesh_selected_index = std::optional<std::uint32_t>{};

        if (::ImGui::BeginCombo("Mesh names", mesh_names_cstr.front()))
        {
            for (const auto &[index, name] : std::views::enumerate(mesh_names_cstr))
            {
                if (::ImGui::Selectable(name))
                {
                    mesh_selected_index = index;
                }
            }
            ::ImGui::EndCombo();
        }

        if (mesh_selected_index)
        {
            scene.create_entity(names[*mesh_selected_index]);
        }

        for (auto &entity : scene.entities())
        {
            ::ImGui::CollapsingHeader(entity.name().c_str());
        }

        for (const auto &[index, light] : std::views::enumerate(scene.lights().lights))
        {
            const auto light_name = std::format("light {}", index);

            ::ImGui::CollapsingHeader(light_name.c_str());
        }

        float amb_color[3]{};
        std::memcpy(amb_color, &scene.lights().ambient, sizeof(amb_color));

        if (::ImGui::ColorPicker3("ambient light color", amb_color))
        {
            std::memcpy(&scene.lights().ambient, amb_color, sizeof(amb_color));
        }

        const auto camera_transform = scene.camera().data().view;

        ::ImGui::BeginTable("transform", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit);

        for (auto row = 0; row < 4; ++row)
        {
            ::ImGui::TableNextRow();
            for (auto col = 0; col < 4; ++col)
            {
                ::ImGui::TableSetColumnIndex(col);
                ::ImGui::Text("%0.2f", camera_transform[col * 4 + row]);
            }
        }

        ::ImGui::EndTable();

        ::ImGui::End();

        ::ImGui::Begin("Log");
        static auto auto_scroll = true;
        static auto force_scroll_to_bottom = false;

        if (::ImGui::Checkbox("auto scroll", &auto_scroll))
        {
            if (auto_scroll)
            {
                force_scroll_to_bottom = auto_scroll;
            }
        }

        ::ImGui::BeginChild("log output");

        if (auto_scroll && !force_scroll_to_bottom)
        {
            const auto scroll_max = ::ImGui::GetScrollMaxY();
            const auto scroll_current = ::ImGui::GetScrollY();

            if (scroll_max > 0.f && scroll_current < scroll_max)
            {
                auto_scroll = false;
            }
        }

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

        if (auto_scroll)
        {
            ::ImGui::SetScrollHereY(1.f);
        }

        ::ImGui::EndChild();

        force_scroll_to_bottom = false;

        ::ImGui::End();

        ::ImGui::Begin("render_targets");
        static constexpr auto width = 175.f;

        const auto aspect_ratio = static_cast<float>(_window.width()) / static_cast<float>(_window.height());
        for (auto i = 0u; i < _gbuffer_rt.color_attachment_count; ++i)
        {
            const auto tex = scene.texture_manager().texture(_gbuffer_rt.first_color_attachment_index + i);
            ::ImGui::Image(tex->native_handle(), ::ImVec2(width * aspect_ratio, width), ::ImVec2(0.f, 1.f), ::ImVec2(1.f, 0.f));
            if ((i + 1) % 4 == 0)
            {
                continue;
            }
            ::ImGui::SameLine();
        }

        ::ImGui::Image(
            scene.texture_manager().texture(_gbuffer_rt.depth_attachment_index)->native_handle(),
            ::ImVec2(width * aspect_ratio, width),
            ::ImVec2(0.f, 1.f),
            ::ImVec2(1.f, 0.f));

        ::ImGui::End();

        if (!std::holds_alternative<std::monostate>(_selected))
        {
            ::ImGui::Begin("inspector");

            if (auto **selected_entity = std::get_if<Entity *>(&_selected))
            {
                auto *entity = *selected_entity;
                ::ImGui::Text("entity: %s", entity->name().c_str());

                auto transform = Matrix4{entity->transform()};

                ::ImGui::BeginTable("transform", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit);

                for (auto row = 0; row < 4; ++row)
                {
                    ::ImGui::TableNextRow();
                    for (auto col = 0; col < 4; ++col)
                    {
                        ::ImGui::TableSetColumnIndex(col);
                        ::ImGui::Text("%0.2f", camera_transform[col * 4 + row]);
                    }
                }
                ::ImGui::EndTable();

                const auto &camera_data = scene.camera().data();
                static float snap_translation[3] = {1.f, 1.f, 1.f};

                ::ImGuizmo::Manipulate(
                    camera_data.view.data().data(),
                    camera_data.projection.data().data(),
                    ::ImGuizmo::TRANSLATE | ::ImGuizmo::SCALE | ::ImGuizmo::ROTATE,
                    ::ImGuizmo::WORLD,
                    const_cast<float *>(transform.data().data()),
                    nullptr,
                    snap_translation,
                    nullptr,
                    nullptr);

                entity->set_transform(transform);
            }
            else if (auto **selected_light = std::get_if<PointLight *>(&_selected); selected_light)
            {
                auto *light = *selected_light;

                ::ImGui::Text("point light");

                float pos[3] = {light->position.x, light->position.y, light->position.z};

                if (::ImGui::SliderFloat3("position", pos, -100.f, 100.f))
                {
                    light->position = {pos[0], pos[1], pos[2]};
                }

                float color[3]{};
                std::memcpy(color, &light->color, sizeof(color));

                if (::ImGui::ColorPicker3("light color", color))
                {
                    std::memcpy(&light->color, color, sizeof(color));
                }

                ::ImGui::SliderFloat("power", &light->specular_poewr, 0.f, 128.f);

                float att[3] = {light->constant_attenuation, light->linear_attenuation, light->quadratic_attenuation};

                if (::ImGui::SliderFloat3("attenuation", att, 0.f, 2.f))
                {
                    light->constant_attenuation = att[0];
                    light->linear_attenuation = att[1];
                    light->quadratic_attenuation = att[2];
                }

                auto transform = Matrix4{light->position};
                const auto &camera_data = scene.camera().data();

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
                light->position = new_transform.position;
            }

            ::ImGui::End();
        }

        ::ImGui::Render();
        ::ImGui_ImplOpenGL3_RenderDrawData(::ImGui::GetDrawData());

        if (_click)
        {
            const auto pick_ray = screen_ray(_click.value(), _window, scene.camera());
            const auto intersection = scene.intersect_ray(pick_ray);
            if (intersection)
            {
                _selected = intersection->entity;
            }
            else
            {
                _selected = std::monostate{};
            }
            for (auto &light : scene.lights().lights)
            {
                const auto light_transform = Transform{light.position, {debugl_light_scale}, {}};
                const auto light_model = Matrix4{light_transform};

                const auto debug_light_aabb = ufps::AABB{
                    .min = light_model * Vector4{-1.f, -1.f, -1.f, 1.f},
                    .max = light_model * Vector4{1.f}};

                if (const auto light_intersection = intersect(pick_ray, debug_light_aabb); light_intersection)
                {
                    if (!intersection || light_intersection < intersection->distance)
                    {
                        _selected = std::addressof(light);
                    }
                }
            }

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