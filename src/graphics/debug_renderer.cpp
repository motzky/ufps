#include "graphics/debug_renderer.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <meta>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>

#include <imgui.h>

#include <ImGuizmo.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "core/camera_manager.h"
#include "core/entity_manager.h"
#include "core/light_manager.h"
#include "core/scene.h"
#include "core/service_locator.h"
#include "events/key.h"
#include "events/mouse_button_event.h"
#include "graphics/mesh_manager.h"
#include "graphics/point_light.h"
#include "graphics/texture_manager.h"
#include "log.h"
#include "math/aabb.h"
#include "math/bounded_number.h"
#include "math/matrix4.h"
#include "math/ray.h"
#include "math/transform.h"
#include "physics/physics_debug_renderer.h"
#include "physics/physics_system.h"
#include "serialization/yaml_serializer.h"

#include "window.h"

using namespace std::literals;

namespace
{
    static constexpr auto debug_light_scale = 0.25f;

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

    auto draw_frustrum(
        const ufps::Vector3 &cam_pos,
        const ufps::Matrix4 &view,
        const ufps::Matrix4 &proj,
        const ufps::Color &color,
        float debug_distance = 3.f) -> std::vector<ufps::LineData>
    {
        auto lines = std::vector<ufps::LineData>{};

        const auto inv_vp = ufps::Matrix4::invert(proj * view);

        constexpr auto ndc_far_corners = std::array<ufps::Vector4, 4>{{{-1.f, 1.f, 1.f, 1.f},
                                                                       {1.f, 1.f, 1.f, 1.f},
                                                                       {1.f, -1.f, 1.f, 1.f},
                                                                       {-1.f, -1.f, 1.f, 1.f}}};

        auto far_corners = std::array<ufps::Vector3, 4>{};
        for (auto i = 0zu; i < 4zu; ++i)
        {
            const auto world_h = inv_vp * ndc_far_corners[i];
            const auto world_pos = ufps::Vector3(world_h) / world_h.w;
            const auto ray_dir = ufps::Vector3::normalize(world_pos - cam_pos);

            far_corners[i] = cam_pos + ray_dir * debug_distance;
        }

        for (auto i = 0zu; i < 4zu; ++i)
        {
            draw_line(cam_pos, far_corners[i], color, lines);
        }

        for (auto i = 0zu; i < 4zu; ++i)
        {
            draw_line(far_corners[i], far_corners[(i + 1) % 4], color, lines);
        }

        return lines;
    }

    template <float Min, float Max>
    auto create_debug_control(const std::string &label, ufps::BoundedFloat<Min, Max> &value) -> void
    {
        ::ImGui::SliderFloat(label.c_str(), &value, Min, Max);
    }

    template <std::uint32_t Min, std::uint32_t Max>
    auto create_debug_control(const std::string &label, ufps::BoundedUint32<Min, Max> &value) -> void
    {
        auto v = static_cast<int>(*value);

        if (::ImGui::SliderInt(label.c_str(), &v, Min, Max))
        {
            value = static_cast<std::uint32_t>(v);
        }
    }

    auto create_debug_control(const std::string &label, ufps::Color &value) -> void
    {
        float v[3]{};
        std::memcpy(v, &value, sizeof(v));

        if (::ImGui::ColorPicker3(label.c_str(), v))
        {
            std::memcpy(&value, v, sizeof(value));
        }
    }

    auto create_debug_control(const std::string &, const ufps::Matrix4 &value) -> void
    {
        ::ImGui::BeginTable("transform", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit);

        for (auto row = 0; row < 4; ++row)
        {
            ::ImGui::TableNextRow();
            for (auto col = 0; col < 4; ++col)
            {
                ::ImGui::TableSetColumnIndex(col);
                ::ImGui::Text("%0.2f", value[col * 4 + row]);
            }
        }

        ::ImGui::EndTable();
    }
}

namespace ufps
{
    DebugRenderer::DebugRenderer(const Window &window, ResourceLoader &resource_loader)
        : Renderer{window, resource_loader},
          _enabled{false},
          _snap_enabled{false},
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
                                              "debug_light_fragment_shader")},
          _highlight_render_entity{},
          _highlight_entity{},
          _highlight_rigid_body{}
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

    auto DebugRenderer::post_render(Scene &scene, const Camera &camera) -> void
    {
        auto &&[em, rem, mm, lm, ps, cm] = services<EntityManager, RenderEntityManager, MeshManager, LightManager, PhysicsSystem, CameraManager>();

        if (std::holds_alternative<EntityHandle>(_selected))
        {
            const auto selected_entity = std::get<EntityHandle>(_selected);
            auto entity = em[selected_entity];
            contract_assert(entity);

            auto aabb_lines =
                entity->render_entities() |
                std::views::transform([&](const auto &e)
                                      { 
                                        auto re = rem[e];
                                        const auto color = e == _highlight_render_entity ? Color::magenta() : Color{0.f, 0.2f, 0.f};
                                        return create_aabb_lines(re->aabb(), entity->transform(), color); }) |
                std::views::join;

            _debug_lines.append_range(aabb_lines);

            const auto color = selected_entity == _highlight_entity ? Color::magenta() : Color::green();

            _debug_lines.append_range(create_aabb_lines(entity->aabb(), entity->transform(), color));
        }

        if (_highlight_entity)
        {
            auto entity = em[_highlight_entity];
            contract_assert(entity);
            _debug_lines.append_range(create_aabb_lines(entity->aabb(), entity->transform(), Color::magenta()));
        }

        if (_highlight_rigid_body)
        {
            const auto rb = ps.rigid_body(_highlight_rigid_body);
            contract_assert(rb);

            _debug_lines.append_range(create_aabb_lines({.min = {-1.f}, .max = {1.f}}, rb->transform(), Color::magenta()));
        }

        Renderer::post_render(scene, camera);

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

        _debug_light_program.bind();

        const auto [vertex_buffer_handle, index_buffer_handle] = mm.native_handle();
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));
        ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_handle);

        const auto cube_parts = mm.mesh("cube");
        ensure(cube_parts.size() == 1u, "cube mesh should have exactly 1 part");
        const auto cube_indices_offset_bytes = cube_parts.front().index_offset * sizeof(std::uint32_t);
        const auto cube_vertex_offset = cube_parts.front().vertex_offset;

        for (const auto &light : lm.data())
        {
            const auto light_transform = Transform{light.position, {debug_light_scale}, {}};
            const auto light_model = Matrix4{light_transform};

            const auto debug_light_aabb = ufps::AABB{
                .min = light_model * Vector4{-1.f, -1.f, -1.f, 1.f},
                .max = light_model * Vector4{1.f}};

            _debug_lines.append_range(create_aabb_lines(debug_light_aabb, {}, {1.f, 0.f, 0.f}));

            _debug_light_program.set_uniforms(light_model, light.color);

            ::glDrawElementsBaseVertex(GL_TRIANGLES, 36, GL_UNSIGNED_INT, reinterpret_cast<const void *>(cube_indices_offset_bytes), cube_vertex_offset);
        }

        for (const auto entity_handle : scene.entities())
        {
            const auto entity = em[entity_handle];
            if (!entity || entity->name() == "flycam")
            {
                continue;
            }

            const auto light_transform = Transform{entity->transform().position, {debug_light_scale / 4.f}, {}};
            const auto light_model = Matrix4{light_transform};

            _debug_light_program.set_uniforms(light_model, Color::yellow());

            ::glDrawElementsBaseVertex(
                GL_TRIANGLES,
                36,
                GL_UNSIGNED_INT,
                reinterpret_cast<const void *>(cube_indices_offset_bytes),
                cube_vertex_offset);

            const auto camera = cm[entity->camera()];
            if (camera)
            {
                const auto camera_transform = Transform{camera->transform().position, {debug_light_scale / 4.f}, camera->transform().rotation};
                const auto camera_model = Matrix4{camera_transform};

                _debug_light_program.set_uniforms(camera_model, Color::azure());

                const auto camera_data = camera->data();

                _debug_lines.append_range(draw_frustrum(camera_data.position, camera_data.view, camera_data.projection, Color::azure(), 1.5f));

                ::glDrawElementsBaseVertex(
                    GL_TRIANGLES,
                    36,
                    GL_UNSIGNED_INT,
                    reinterpret_cast<const void *>(cube_indices_offset_bytes),
                    cube_vertex_offset);
            }
        }

        _debug_light_program.unbind();

        auto &&physics_debug_renderer = ps.debug_renderer();
        if (physics_debug_renderer)
        {
            _debug_lines.append_range(physics_debug_renderer->yield_lines());
        }

        if (!_debug_lines.empty())
        {
            _debug_line_program.bind();

            resize_gpu_buffer(_debug_lines, _debug_line_buffer);
            _debug_line_buffer.write(std::as_bytes(std::span{_debug_lines.data(), _debug_lines.size()}), 0zu);
            ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _debug_line_buffer.native_handle());
            ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));

            ::glDrawArrays(GL_LINES, 0, _debug_lines.size());

            _debug_line_program.unbind();

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

        draw_post_processing_window(scene);
        draw_scene(scene, camera);
        draw_logs();
        draw_bloom_mips();
        draw_render_targets();
        // draw_metrics();
        draw_inspector(scene);
        draw_gizmo(camera);

        static auto first = false;
        if (!first)
        {
            ::ImGui::SetWindowFocus("Render Targets");
            ::ImGui::SetWindowFocus("Scene");
            first = false;
        }

        ::ImGui::Render();
        ::ImGui_ImplOpenGL3_RenderDrawData(::ImGui::GetDrawData());

        if (_click)
        {
            const auto pick_ray = screen_ray(_click.value(), _window, camera);
            auto intersection = scene.intersect_ray(pick_ray);
            if (intersection)
            {
                _selected = intersection->entity;
            }
            else
            {
                _selected = std::monostate{};
            }
            for (auto light_handle : lm.handles())
            {
                const auto light = lm[light_handle];
                if (!light)
                {
                    continue;
                }
                const auto light_transform = Transform{light->position, {debug_light_scale}, {}};
                const auto light_model = Matrix4{light_transform};

                const auto debug_light_aabb = ufps::AABB{
                    .min = light_model * Vector4{-1.f, -1.f, -1.f, 1.f},
                    .max = light_model * Vector4{1.f}};

                if (const auto light_intersection = intersect(pick_ray, debug_light_aabb); light_intersection)
                {
                    if (!intersection || light_intersection < intersection->distance)
                    {
                        _selected = light_handle;
                    }
                }
            }

            for (const auto entity_handle : scene.entities())
            {
                const auto entity = em[entity_handle];
                if (!entity || entity->name() == "flycam")
                {
                    continue;
                }

                const auto entity_transform = Transform{entity->transform().position, {debug_light_scale / 4.f}, {}};
                const auto entity_model = Matrix4{entity_transform};

                const auto debug_entity_aabb = ufps::AABB{
                    .min = entity_model * Vector4{-1.f, -1.f, -1.f, 1.f},
                    .max = entity_model * Vector4{1.f}};

                if (const auto light_intersection = intersect(pick_ray, debug_entity_aabb); light_intersection)
                {
                    if (!intersection || light_intersection < intersection->distance)
                    {
                        _selected = entity_handle;
                    }
                }

                const auto camera = cm[entity->camera()];
                if (camera)
                {
                    const auto camera_transform = Transform{camera->transform().position, {debug_light_scale / 4.f}, camera->transform().rotation};
                    const auto camera_model = Matrix4{camera_transform};
                    const auto debug_entity_aabb = ufps::AABB{
                        .min = camera_model * Vector4{-1.f, -1.f, -1.f, 1.f},
                        .max = camera_model * Vector4{1.f}};

                    if (const auto camera_intersection = intersect(pick_ray, debug_entity_aabb); camera_intersection)
                    {
                        if (!intersection || camera_intersection < intersection->distance)
                        {
                            _selected = entity_handle;
                        }
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

    auto DebugRenderer::add_key_event(const KeyEvent &evt) -> void
    {
        auto &io = ::ImGui::GetIO();

        switch (evt.key())
        {
            using enum Key;
        case BACKSPACE:
            io.AddKeyEvent(::ImGuiKey_Backspace, evt.state() == KeyState::DOWN);
            break;
        case LSHIFT:
        case RSHIFT:
            break;
        default:
            if (evt.state() == KeyState::DOWN)
            {
                io.AddInputCharacter(std::to_underlying(evt.key()));
            }

            break;
        }
    }

    auto DebugRenderer::set_enabled(bool enabled) -> void
    {
        _enabled = enabled;
        _enable_post_processing = !enabled;
    }

    auto DebugRenderer::draw_post_processing_window(Scene &scene) -> void
    {
        if (::ImGui::Begin("Post Processing"))
        {
            ::ImGui::PushID("post_processing");

            ::ImGui::Checkbox("enable post processing", std::addressof(_enable_post_processing));

            if (::ImGui::CollapsingHeader("Tone Map Options"))
            {
                ::ImGui::PushID("tone_map_options");

                create_debug_control("max_brightness", scene.tone_map_options().max_brightness);
                create_debug_control("contrast", scene.tone_map_options().contrast);
                create_debug_control("linear_section_start", scene.tone_map_options().linear_section_start);
                create_debug_control("linear_section_length", scene.tone_map_options().linear_section_length);
                create_debug_control("black_tightness", scene.tone_map_options().black_tightness);
                create_debug_control("pedestal", scene.tone_map_options().pedestal);
                create_debug_control("gamma", scene.tone_map_options().gamma);

                ::ImGui::PopID();
            }

            if (::ImGui::CollapsingHeader("SSAO Options"))
            {
                ::ImGui::PushID("ssao_options");

                ::ImGui::Checkbox("enabled", std::addressof(scene.ssao_options().enabled));
                create_debug_control("sample_count", scene.ssao_options().sample_count);
                create_debug_control("radius", scene.ssao_options().radius);
                create_debug_control("bias", scene.ssao_options().bias);
                create_debug_control("power", scene.ssao_options().power);

                ::ImGui::PopID();
            }

            if (::ImGui::CollapsingHeader("Bloom Options"))
            {
                ::ImGui::PushID("bloom_options");

                create_debug_control("filter_radius", scene.bloom_options().filter_radius);
                create_debug_control("mix_amount", scene.bloom_options().mix_amount);
                create_debug_control("threshold", scene.bloom_options().threshold);

                ::ImGui::PopID();
            }

            if (::ImGui::CollapsingHeader("Fog Options"))
            {
                ::ImGui::PushID("fog_options");

                create_debug_control("color", scene.fog_options().color);
                create_debug_control("density", scene.fog_options().density);

                ::ImGui::PopID();
            }
            if (::ImGui::CollapsingHeader("Chromatic Aberration Options"))
            {
                ::ImGui::PushID("chromatic_aberration_options");

                create_debug_control("red", scene.chromatic_abberation_options().red_offset);
                create_debug_control("green", scene.chromatic_abberation_options().green_offset);
                create_debug_control("blue", scene.chromatic_abberation_options().blue_offset);
                create_debug_control("strength", scene.chromatic_abberation_options().strength);

                ::ImGui::PopID();
            }

            if (::ImGui::CollapsingHeader("Vignette Options"))
            {
                ::ImGui::PushID("vignette_options");

                create_debug_control("colour", scene.vignette_options().color);
                create_debug_control("strength", scene.vignette_options().strength);
                create_debug_control("feather", scene.vignette_options().feather);

                ::ImGui::PopID();
            }

            if (::ImGui::CollapsingHeader("Film Grain Options"))
            {
                ::ImGui::PushID("film_grain_options");

                create_debug_control("strength", scene.film_grain_options().strength);

                ::ImGui::PopID();
            }

            ::ImGui::PopID();
        }
        ::ImGui::End();
    }

    auto DebugRenderer::draw_scene(Scene &scene, const Camera &camera) -> void
    {
        auto &em = service<EntityManager>();

        if (::ImGui::Begin("Scene"))
        {
            ::ImGui::PushID("scene");

            if (::ImGui::Button("Save"))
            {
                const auto scene_yaml = ufps::yaml::serialize(scene.description());
                contract_assert(scene_yaml);
                auto out = std::ofstream("scene.yaml");

                out << *scene_yaml;
            }

            if (::ImGui::Button("Add Entity"))
            {
                static auto counter = std::ranges::size(scene.entities());
                const auto handle = em.insert({std::format("entity_{}", counter++), {}, {}});
                scene.add(handle);
                _selected = handle;
            }

            ::ImGui::Text("ambient");
            create_debug_control("ambient", scene.ambient_light());

            ::ImGui::Text("camera_view");
            create_debug_control("camera_view", camera.data().view);

            ::ImGui::Text("entities");
            const auto entities = scene.entities() |
                                  std::views::transform(
                                      [&](const auto &e)
                                      {
                                          const auto entity = em[e];
                                          contract_assert(entity);
                                          return std::make_tuple(entity->name(), e);
                                      }) |
                                  std::ranges::to<std::vector>();

            if (::ImGui::BeginListBox("entities"))
            {
                static auto selected_index = std::optional<std::size_t>{};

                auto to_highlight = EntityHandle{};

                for (const auto &[index, entity] : std::views::enumerate(entities))
                {
                    const auto &[name, handle] = entity;
                    const auto is_selected = selected_index == index;
                    const auto flags = is_selected ? ::ImGuiSelectableFlags_Highlight : 0;

                    if (::ImGui::Selectable(name.data(), is_selected, flags))
                    {
                        selected_index = index;
                        _selected = handle;
                    }

                    if (::ImGui::IsItemHovered())
                    {
                        to_highlight = handle;
                    }

                    if (is_selected)
                    {
                        ::ImGui::SetItemDefaultFocus();
                    }
                }

                _highlight_entity = to_highlight;

                ::ImGui::EndListBox();
            }

            auto average_luminance = 0.f;
            ::glGetNamedBufferSubData(_average_luminance_buffer.native_handle(), 0, sizeof(average_luminance), &average_luminance);

            std::uint32_t histogram[256]{};
            ::glGetNamedBufferSubData(_luminance_histogram_buffer.native_handle(), 0, sizeof(histogram), &histogram);

            const auto scaled_histogram =
                histogram |
                std::views::transform([](const auto e)
                                      { return std::log2(static_cast<float>(e) + 1.f); }) |
                std::ranges::to<std::vector>();

            ::ImGui::LabelText("average_luminance", "%0.2f", average_luminance);
            ::ImGui::PlotHistogram(
                "luminance_histogram",
                scaled_histogram.data(),
                256,
                0,
                nullptr,
                0.f,
                std::ranges::max(scaled_histogram),
                ::ImVec2(::ImGui::GetContentRegionAvail().x, 150.f));

            ::ImGui::PopID();
        }

        ::ImGui::End();
    }

    auto DebugRenderer::draw_logs() -> void
    {
        if (::ImGui::Begin("Logs"))
        {
            ::ImGui::PushID("logs");

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

            for (const auto &pair : ufps::log::history)
            {
                switch (pair.first)
                {
                    using enum ufps::log::Level;
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

            ::ImGui::PopID();
        }

        ::ImGui::End();
    }

    auto DebugRenderer::draw_bloom_mips() -> void
    {
        auto &tm = service<TextureManager>();

        if (::ImGui::Begin("Bloom Mips"))
        {
            ::ImGui::PushID("bloom_mips");

            static constexpr auto width = 175.f;
            const auto aspect_ratio = static_cast<float>(_window.width()) / static_cast<float>(_window.height());

            for (const auto &[index, mip] : std::views::enumerate(_bloom_mips))
            {
                ::ImGui::Image(
                    tm.texture(mip.color_texture_bindless_handle_0)->native_handle(),
                    ::ImVec2(width * aspect_ratio, width),
                    ::ImVec2(0.f, 1.f),
                    ::ImVec2(1.f, 0.f));

                if ((index + 1) % 4 != 0)
                {
                    ::ImGui::SameLine();
                }
            }

            ::ImGui::PopID();
        }

        ::ImGui::End();
    }

    auto DebugRenderer::draw_render_targets() -> void
    {
        auto &tm = service<TextureManager>();

        if (::ImGui::Begin("Render Targets"))
        {
            ::ImGui::PushID("render_targets");

            static constexpr auto width = 175.f;
            const auto aspect_ratio = static_cast<float>(_window.width()) / static_cast<float>(_window.height());

            ::ImGui::Image(
                tm.texture(_gbuffer_rt.color_texture_bindless_handle_0)->native_handle(),
                ::ImVec2(width * aspect_ratio, width),
                ::ImVec2(0.f, 1.f),
                ::ImVec2(1.f, 0.f));
            ::ImGui::SameLine();

            ::ImGui::Image(
                tm.texture(_gbuffer_rt.color_texture_bindless_handle_1)->native_handle(),
                ::ImVec2(width * aspect_ratio, width),
                ::ImVec2(0.f, 1.f),
                ::ImVec2(1.f, 0.f));
            ::ImGui::SameLine();

            ::ImGui::Image(
                tm.texture(_gbuffer_rt.color_texture_bindless_handle_2)->native_handle(),
                ::ImVec2(width * aspect_ratio, width),
                ::ImVec2(0.f, 1.f),
                ::ImVec2(1.f, 0.f));
            ::ImGui::SameLine();

            ::ImGui::Image(
                tm.texture(_gbuffer_rt.color_texture_bindless_handle_3)->native_handle(),
                ::ImVec2(width * aspect_ratio, width),
                ::ImVec2(0.f, 1.f),
                ::ImVec2(1.f, 0.f));

            ::ImGui::Image(
                tm.texture(_gbuffer_rt.color_texture_bindless_handle_4)->native_handle(),
                ::ImVec2(width * aspect_ratio, width),
                ::ImVec2(0.f, 1.f),
                ::ImVec2(1.f, 0.f));
            ::ImGui::SameLine();

            ::ImGui::Image(
                tm.texture(_gbuffer_rt.color_texture_bindless_handle_4)->native_handle(),
                ::ImVec2(width * aspect_ratio, width),
                ::ImVec2(0.f, 1.f),
                ::ImVec2(1.f, 0.f));
            ::ImGui::SameLine();

            ::ImGui::Image(
                tm.texture(_gbuffer_rt.color_texture_bindless_handle_5)->native_handle(),
                ::ImVec2(width * aspect_ratio, width),
                ::ImVec2(0.f, 1.f),
                ::ImVec2(1.f, 0.f));
            ::ImGui::SameLine();

            ::ImGui::Image(
                tm.texture(_gbuffer_rt.color_texture_bindless_handle_6)->native_handle(),
                ::ImVec2(width * aspect_ratio, width),
                ::ImVec2(0.f, 1.f),
                ::ImVec2(1.f, 0.f));

            ::ImGui::Image(
                tm.texture(_ssao_blur_rt.color_texture_bindless_handle_0)->native_handle(),
                ::ImVec2(width * aspect_ratio, width),
                ::ImVec2(0.f, 1.f),
                ::ImVec2(1.f, 0.f));
            ::ImGui::SameLine();

            ::ImGui::Image(
                tm.texture(_forward_transparancy_rt.color_texture_bindless_handle_0)->native_handle(),
                ::ImVec2(width * aspect_ratio, width),
                ::ImVec2(0.f, 1.f),
                ::ImVec2(1.f, 0.f));
            ::ImGui::SameLine();

            ::ImGui::Image(
                tm.texture(_bloom_rt.color_texture_bindless_handle_0)->native_handle(),
                ::ImVec2(width * aspect_ratio, width),
                ::ImVec2(0.f, 1.f),
                ::ImVec2(1.f, 0.f));

            ::ImGui::PopID();
        }

        ::ImGui::End();
    }

    auto DebugRenderer::draw_inspector(Scene &scene) -> void
    {

        if (!std::holds_alternative<std::monostate>(_selected))
        {
            if (::ImGui::Begin("Inspector"))
            {
                const auto &[tm, em, rem, ps, lm, cm] = services<TextureManager, EntityManager, RenderEntityManager, PhysicsSystem, LightManager, CameraManager>();

                if (auto *selected_entity = std::get_if<EntityHandle>(&_selected))
                {
                    auto entity = em[*selected_entity];
                    contract_assert(entity);

                    ::ImGui::PushStyleColor(ImGuiCol_Button, ::ImVec4(0.8f, 0.15f, 0.15f, 1.f));
                    ::ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ::ImVec4(0.95f, 0.25f, 0.25f, 1.f));
                    ::ImGui::PushStyleColor(ImGuiCol_ButtonActive, ::ImVec4(0.65f, 0.1f, 0.1f, 1.f));

                    if (::ImGui::Button("Delete"))
                    {
                        scene.remove(*selected_entity);
                        _selected = std::monostate{};
                        ::ImGui::PopStyleColor(3);
                        ::ImGui::End();

                        return;
                    }

                    ::ImGui::PopStyleColor(3);

                    ::ImGui::Text("entity");
                    ::ImGui::SameLine();

                    static auto buffer = std::array<char, 256>{};
                    const auto name = entity->name();
                    ensure(std::ranges::size(name) < std::ranges::size(buffer), "{} too long", name);
                    std::ranges::copy(std::ranges::begin(name), std::ranges::end(name) + 1zu, std::ranges::begin(buffer));

                    ::ImGui::InputText("Name", std::ranges::data(buffer), std::ranges::size(buffer));
                    ::ImGui::SameLine();

                    if (::ImGui::Button("Set Name"))
                    {
                        entity->set_name(std::string(std::ranges::data(buffer)));
                        log::debug("new name: {}", std::ranges::data(buffer));
                    }

                    ::ImGui::Checkbox("Enable snap", std::addressof(_snap_enabled));

                    if (::ImGui::Button("Add Child Entity"))
                    {
                        static auto counter = 0zu;
                        const auto handle = em.insert({std::format("{}_child_{}", entity->name(), counter++), {}, {}});

                        entity->add_child(handle);
                        scene.add(handle);
                        _selected = handle;
                    }

                    {
                        auto to_delete = EntityHandle{};
                        auto to_highlight = EntityHandle{};
                        auto to_duplicate = EntityHandle{};

                        for (const auto &[index, handle] : std::views::enumerate(entity->children()))
                        {
                            const auto child = em[handle];
                            contract_assert(child);

                            const auto header = std::format("child_entity {}", index);

                            if (::ImGui::CollapsingHeader(header.c_str()))
                            {
                                ::ImGui::PushID(index);

                                ::ImGui::PushStyleColor(ImGuiCol_Button, ::ImVec4(0.8f, 0.15f, 0.15f, 1.f));
                                ::ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ::ImVec4(0.95f, 0.25f, 0.25f, 1.f));
                                ::ImGui::PushStyleColor(ImGuiCol_ButtonActive, ::ImVec4(0.65f, 0.1f, 0.1f, 1.f));

                                if (::ImGui::Button("Delete"))
                                {
                                    to_delete = handle;
                                }

                                ::ImGui::PopStyleColor(3);

                                ::ImGui::SameLine();

                                if (::ImGui::Button("Select"))
                                {
                                    _selected = handle;
                                }

                                ::ImGui::SameLine();

                                if (::ImGui::Button("Duplicate"))
                                {
                                    to_duplicate = handle;
                                }

                                ::ImGui::PopID();
                            }

                            if (::ImGui::IsItemHovered())
                            {
                                log::debug("hover {}", index);
                                to_highlight = handle;
                            }
                        }

                        if (to_delete)
                        {
                        }

                        if (to_duplicate)
                        {
                            const auto child = em[to_duplicate];
                            static int counter = 0;

                            const auto name = std::format("{}_copy_{}", child->name(), counter++);
                            const auto handle = em.insert({name, child->render_entities(), child->transform()});
                            scene.add(handle);
                            entity->add_child(handle);
                            _selected = handle;
                        }

                        _highlight_entity = to_highlight;
                    }

                    auto groups = rem.groups() | std::ranges::to<std::vector>();
                    std::ranges::sort(groups);

                    if (::ImGui::BeginCombo("add render entity group", "Select one", 0))
                    {
                        for (const auto &name : groups)
                        {
                            if (::ImGui::Selectable(name.c_str(), false))
                            {
                                entity->add_render_entities(rem[name]);
                            }
                        }

                        ::ImGui::EndCombo();
                    }

                    {
                        auto to_delete = RenderEntityHandle{};
                        auto to_highlight = RenderEntityHandle{};

                        auto debug_draw_texture = [&tm](auto idx, auto should_same_line) -> void
                        {
                            if (idx < 65537)
                            {
                                const auto *texture = tm.texture(idx);
                                if (texture)
                                {
                                    ::ImGui::Image(
                                        texture->native_handle(),
                                        ::ImVec2(64.f, 64.f),
                                        ::ImVec2(0.f, 1.f),
                                        ::ImVec2(1.f, 0.f));
                                    if (should_same_line)
                                    {
                                        ::ImGui::SameLine();
                                    }
                                }
                            }
                        };

                        for (const auto &[index, handle] : std::views::enumerate(entity->render_entities()))
                        {
                            auto render_entity = rem[handle];

                            const auto header = std::format("render_entity {}", index);

                            if (::ImGui::CollapsingHeader(header.c_str()))
                            {
                                ::ImGui::PushID(index);

                                ::ImGui::PushStyleColor(ImGuiCol_Button, ::ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
                                ::ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ::ImVec4(0.95f, 0.25f, 0.25f, 1.0f));
                                ::ImGui::PushStyleColor(ImGuiCol_ButtonActive, ::ImVec4(0.65f, 0.1f, 0.1f, 1.0f));

                                if (::ImGui::Button("Delete"))
                                {
                                    to_delete = handle;
                                }

                                ::ImGui::PopStyleColor(3);

                                debug_draw_texture(render_entity->albedo_texture_bindless_handle(), true);
                                debug_draw_texture(render_entity->normal_texture_bindless_handle(), true);
                                debug_draw_texture(render_entity->specular_texture_bindless_handle(), false);
                                debug_draw_texture(render_entity->roughness_texture_bindless_handle(), true);
                                debug_draw_texture(render_entity->ao_texture_bindless_handle(), true);
                                debug_draw_texture(render_entity->emissive_texture_bindless_handle(), false);

                                ::ImGui::PopID();
                            }

                            if (::ImGui::IsItemHovered())
                            {
                                to_highlight = handle;
                            }
                        }

                        if (to_delete)
                        {
                            entity->remove_render_entity(to_delete);
                        }

                        _highlight_render_entity = to_highlight;
                    }

                    if (::ImGui::Button("add rigid body"))
                    {
                        const auto body = service<PhysicsSystem>().create_box({{-1.f}, {1.f}}, entity->transform().position, ufps::PhysicsLayer::DYNAMIC);
                        entity->add_rigid_body(body);
                        _selected = body;
                    }

                    {
                        auto to_delete = RigidBodyHandle{};
                        auto to_duplicate = RigidBodyHandle{};
                        auto to_highlight = RigidBodyHandle{};

                        for (const auto &[index, handle] : std::views::enumerate(entity->rigid_bodies()))
                        {
                            const auto header = std::format("rigid body {}", index);
                            if (::ImGui::CollapsingHeader(header.c_str()))
                            {
                                ::ImGui::PushID(index);

                                if (::ImGui::Button("Select"))
                                {
                                    _selected = handle;
                                }

                                ::ImGui::SameLine();

                                ::ImGui::PushStyleColor(ImGuiCol_Button, ::ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
                                ::ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ::ImVec4(0.95f, 0.25f, 0.25f, 1.0f));
                                ::ImGui::PushStyleColor(ImGuiCol_ButtonActive, ::ImVec4(0.65f, 0.1f, 0.1f, 1.0f));

                                if (::ImGui::Button("Delete"))
                                {
                                    to_delete = handle;
                                }

                                ::ImGui::PopStyleColor(3);

                                ::ImGui::SameLine();

                                if (::ImGui::Button("Duplicate"))
                                {
                                    to_duplicate = handle;
                                }

                                ::ImGui::PopID();
                            }

                            if (::ImGui::IsItemHovered())
                            {
                                to_highlight = handle;
                            }
                        }

                        if (to_delete)
                        {
                            ps.remove_rigid_body(to_delete);
                        }

                        if (to_duplicate)
                        {
                            const auto handle = ps.duplicate_rigid_body(to_duplicate);
                            entity->add_rigid_body(handle);
                            _selected = handle;
                        }

                        _highlight_rigid_body = to_highlight;

                        {
                            auto value = entity->emissive_strength();
                            if (::ImGui::SliderFloat("emissive_strength", &value, 0.f, 10.f))
                            {
                                entity->set_emissive_strength(value);
                            }
                        }

                        const auto transforms = std::array<Matrix4, 3u>{
                            entity->transform(),
                            entity->local_transform(),
                            entity->parent_transform()};
                        const auto transform_names = std::array<std::string, 3u>{"world", "local", "parent"};

                        for (const auto &[transform, name] : std::views::zip(transforms, transform_names))
                        {
                            ::ImGui::Text(name.c_str());
                            ::ImGui::SameLine();

                            ::ImGui::BeginTable(name.c_str(), 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit);

                            for (auto row = 0; row < 4; ++row)
                            {
                                ::ImGui::TableNextRow();
                                for (auto col = 0; col < 4; ++col)
                                {
                                    ::ImGui::TableSetColumnIndex(col);
                                    ::ImGui::Text("%0.2f", transform[col * 4 + row]);
                                }
                            }

                            ::ImGui::EndTable();
                        }
                    }
                    {
                        const auto camera_handle = entity->camera();
                        if (!camera_handle || !cm[camera_handle])
                        {
                            if (::ImGui::Button("Add camera"))
                            {
                                const auto new_handle = cm.insert({{}, std::numbers::pi_v<float> / 4.f, 1920.f, 1080.f, 0.1f, 1000.f});
                                cm[new_handle]->set_parent_transform(entity->transform());

                                entity->set_camera(new_handle);
                                _selected = new_handle;
                            }
                        }
                        else
                        {
                            if (::ImGui::Button("Look through"))
                            {
                                service<CameraHandle>() = camera_handle;
                            }
                            if (::ImGui::Button("Select"))
                            {
                                _selected = camera_handle;
                            }

                            ::ImGui::SameLine();

                            ::ImGui::PushStyleColor(ImGuiCol_Button, ::ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
                            ::ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ::ImVec4(0.95f, 0.25f, 0.25f, 1.0f));
                            ::ImGui::PushStyleColor(ImGuiCol_ButtonActive, ::ImVec4(0.65f, 0.1f, 0.1f, 1.0f));

                            if (::ImGui::Button("Delete"))
                            {
                                entity->set_camera({});
                                cm.remove(camera_handle);
                            }

                            ::ImGui::PopStyleColor(3);
                        }
                    }
                }
                else if (auto *selected_light = std::get_if<LightHandle>(&_selected))
                {
                    auto light = lm[*selected_light];
                    contract_assert(light);

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

                    ::ImGui::SliderFloat("power", &light->specular_power, 0.f, 128.f);

                    float att[3] = {light->constant_attenuation, light->linear_attenuation, light->quadratic_attenuation};

                    if (::ImGui::SliderFloat3("attenuation", att, 0.f, 2.f))
                    {
                        light->constant_attenuation = att[0];
                        light->linear_attenuation = att[1];
                        light->quadratic_attenuation = att[2];
                    }

                    auto intensity = light->intensity;
                    if (::ImGui::SliderFloat("intensity", &intensity, 0.f, 100.f))
                    {
                        light->intensity = intensity;
                    }
                }
                else if (auto *selected_camera = std::get_if<CameraHandle>(&_selected))
                {
                    auto camera = cm[*selected_camera];

                    if (::ImGui::Button("Look through"))
                    {
                        service<CameraHandle>() = *selected_camera;
                    }

                    auto fov = camera->fov();
                    if (::ImGui::SliderFloat("fov", &fov, 0.6f, 1.1f))
                    {
                        camera->set_fov(fov);
                    }

                    const auto transforms = std::array<Matrix4, 3u>{
                        camera->transform(),
                        camera->local_transform(),
                        camera->parent_transform()};
                    const auto transform_names = std::array<std::string, 3u>{"world", "local", "parent"};

                    for (const auto &[transform, name] : std::views::zip(transforms, transform_names))
                    {
                        ::ImGui::Text(name.c_str());
                        ::ImGui::SameLine();

                        ::ImGui::BeginTable(name.c_str(), 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit);

                        for (auto row = 0; row < 4; ++row)
                        {
                            ::ImGui::TableNextRow();
                            for (auto col = 0; col < 4; ++col)
                            {
                                ::ImGui::TableSetColumnIndex(col);
                                ::ImGui::Text("%0.2f", transform[col * 4 + row]);
                            }
                        }

                        ::ImGui::EndTable();
                    }
                }
            }

            ::ImGui::End();
        }
    }

    auto DebugRenderer::draw_gizmo(const Camera &camera) -> void
    {
        const auto &[em, ps, lm, cm] = services<EntityManager, PhysicsSystem, LightManager, CameraManager>();

        if (!std::holds_alternative<std::monostate>(_selected))
        {
            const auto &camera_data = camera.data();

            if (auto *selected_entity = std::get_if<EntityHandle>(&_selected))
            {
                auto entity = em[*selected_entity];
                contract_assert(entity);

                static float snap_translation[3] = {1.f, 1.f, 1.f};
                auto world_matrix = Matrix4{entity->transform()};

                ::ImGuizmo::Manipulate(
                    camera_data.view.data().data(),
                    camera_data.projection.data().data(),
                    ::ImGuizmo::TRANSLATE | ::ImGuizmo::ROTATE,
                    ::ImGuizmo::WORLD,
                    world_matrix.data().data(),
                    nullptr,
                    _snap_enabled ? snap_translation : nullptr,
                    nullptr,
                    nullptr);

                if (::ImGuizmo::IsUsing())
                {
                    const auto parent = Matrix4{entity->parent_transform()};
                    const auto inverse_parent = Matrix4::invert(parent);
                    const auto local = inverse_parent * world_matrix;

                    entity->set_transform(local);
                }
            }
            else if (auto *selected_light = std::get_if<LightHandle>(&_selected))
            {
                auto light = lm[*selected_light];
                contract_assert(light);

                auto transform = Matrix4{light->position};

                ::ImGuizmo::Manipulate(
                    camera_data.view.data().data(),
                    camera_data.projection.data().data(),
                    ::ImGuizmo::TRANSLATE,
                    ::ImGuizmo::WORLD,
                    transform.data().data(),
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr);

                if (::ImGuizmo::IsUsing())
                {
                    const auto new_transform = Transform{transform};
                    light->position = new_transform.position;
                }
            }
            else if (auto *selected_rigid_body = std::get_if<RigidBodyHandle>(&_selected))
            {
                if (auto rigid_body = ps.rigid_body(*selected_rigid_body); rigid_body)
                {
                    auto &rb = *rigid_body;

                    auto world_matrix = Matrix4{rb.transform()};

                    ::ImGuizmo::Manipulate(
                        camera_data.view.data().data(),
                        camera_data.projection.data().data(),
                        ::ImGuizmo::TRANSLATE | ::ImGuizmo::SCALE | ::ImGuizmo::ROTATE,
                        ::ImGuizmo::WORLD,
                        world_matrix.data().data(),
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr);

                    if (::ImGuizmo::IsUsing())
                    {
                        const auto parent = Matrix4{rb.parent_transform()};
                        const auto inverse_parent = Matrix4::invert(parent);
                        const auto local = inverse_parent * world_matrix;

                        rb.set_local_transform(local);
                    }
                }
            }
            else if (auto *selected_camera = std::get_if<CameraHandle>(&_selected))
            {
                auto camera = cm[*selected_camera];
                contract_assert(camera);

                auto world_matrix = Matrix4{camera->transform()};

                ::ImGuizmo::Manipulate(
                    camera_data.view.data().data(),
                    camera_data.projection.data().data(),
                    ::ImGuizmo::TRANSLATE | ::ImGuizmo::ROTATE,
                    ::ImGuizmo::WORLD,
                    world_matrix.data().data(),
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr);

                if (::ImGuizmo::IsUsing())
                {
                    const auto parent = Matrix4{camera->parent_transform()};
                    const auto inverse_parent = Matrix4::invert(parent);
                    const auto local = inverse_parent * world_matrix;

                    camera->set_transform(local);
                }
            }
        }
    }

}