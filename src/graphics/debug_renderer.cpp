#include "graphics/debug_renderer.h"

#include <algorithm>
#include <cstring>
#include <meta>
#include <ranges>
#include <string>
#include <type_traits>

#include <imgui.h>

#include <ImGuizmo.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "core/scene.h"
#include "core/service_locator.h"
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

    struct SaveSceneButton
    {
        ufps::Scene &scene;
    };

    struct AddLightButton
    {
        ufps::Scene &scene;
        std::variant<std::monostate, ufps::Entity *, ufps::PointLightHandle> *selected;
    };

    struct Histogram
    {
        std::vector<float> values;
    };

    struct AddEntity
    {
        ufps::Scene &scene;
        std::variant<std::monostate, ufps::Entity *, ufps::PointLightHandle> *selected;
    };

    struct DuplicateEntity
    {
        ufps::Scene &scene;
        std::variant<std::monostate, ufps::Entity *, ufps::PointLightHandle> *selected;
    };

    struct DeleteEntity
    {
        ufps::Scene &scene;
        std::variant<std::monostate, ufps::Entity *, ufps::PointLightHandle> *selected;
    };

    struct Plot
    {
        std::vector<float> values;
    };

    struct TextureController
    {
        std::uint32_t handle;
        float width;
        float height;
    };

    template <class T>
    struct Wrapper
    {
        T &controller;
    };

    struct SameLine
    {
    };

    struct LogView
    {
    };

    constexpr auto
    clean_name(std::string_view name) -> std::string
    {
        return std::string{name.substr(name.find_last_of(":") + 1)};
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

    auto create_debug_control(const std::string &label, bool &value) -> void
    {
        ::ImGui::Checkbox(label.c_str(), &value);
    }

    auto create_debug_control(const std::string &label, float &value) -> void
    {
        ::ImGui::LabelText(label.c_str(), "%0.2f", value);
    }

    // auto create_debug_control(const std::string &label, std::size_t &value) -> void
    // {
    //     ::ImGui::LabelText(label.c_str(), "%zu", value);
    // }

    auto create_debug_control(const std::string &label, ufps::Color &value) -> void
    {
        float v[3]{};
        std::memcpy(v, &value, sizeof(v));

        if (::ImGui::ColorPicker3(label.c_str(), v))
        {
            std::memcpy(&value, v, sizeof(value));
        }
    }

    auto create_debug_control(const std::string &, SaveSceneButton &value) -> void
    {
        if (::ImGui::Button("save"))
        {
            const auto scene_yaml = ufps::yaml::serialize(value.scene.description());
            auto out = std::ofstream("scene.yaml");
            out << scene_yaml;
        }
    }

    auto create_debug_control(const std::string &, AddLightButton &value) -> void
    {
        if (::ImGui::Button("add light"))
        {
            const auto handle = value.scene.lights().lights.emplace(
                ufps::PointLight{
                    .position = {},
                    .color = {.r = 1.f, .g = 1.f, .b = 1.f},
                    .constant_attenuation = 1.f,
                    .linear_attenuation = 0.007f,
                    .quadratic_attenuation = 0.0002f,
                    .specular_power = 32.f,
                    .intensity = 1.f});
            *value.selected = handle;
        }
    }

    auto create_debug_control(const std::string &label, Histogram &value) -> void
    {
        ::ImGui::PlotHistogram(
            label.c_str(),
            value.values.data(),
            256,
            0,
            nullptr,
            0.0f,
            std::ranges::max(value.values),
            ::ImVec2(::ImGui::GetContentRegionAvail().x, 150.f));
    }

    auto create_debug_control(const std::string &, AddEntity &value) -> void
    {
        auto mesh_selected_index = std::optional<std::uint32_t>{};

        auto mesh_names = ufps::service<ufps::MeshManager>().mesh_names();
        std::ranges::sort(mesh_names);
        const auto mesh_names_str = mesh_names |
                                    std::views::filter([](const auto &e)
                                                       { return !e.empty(); }) |
                                    std::views::transform([](const auto &e)
                                                          { return e.c_str(); }) |
                                    std::ranges::to<std::vector>();

        if (::ImGui::BeginCombo("mesh_names", mesh_names_str.front(), 0))
        {
            for (const auto &[index, name] : std::views::enumerate(mesh_names_str))
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
            value.scene.create_entity(mesh_names_str[*mesh_selected_index]);
            *value.selected = &value.scene.entities().back();
        }
    }

    auto create_debug_control(const std::string &, DeleteEntity &value) -> void
    {
        if (::ImGui::Button("delete"))
        {
            if (auto **selected_entity = std::get_if<ufps::Entity *>(value.selected))
            {
                auto *entity = *selected_entity;
                value.scene.remove(*entity);
                *value.selected = std::monostate{};
            }
            if (auto *selected_light = std::get_if<ufps::PointLightHandle>(value.selected))
            {
                value.scene.lights().lights.remove(*selected_light);
                *value.selected = std::monostate{};
            }
        }
    }

    auto create_debug_control(const std::string &, ufps::Matrix4 &value) -> void
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

    auto create_debug_control(const std::string &, DuplicateEntity &value) -> void
    {
        if (::ImGui::Button("duplicate"))
        {
            if (auto **selected_entity = std::get_if<ufps::Entity *>(value.selected))
            {
                auto *entity = *selected_entity;
                auto *new_entity = value.scene.create_entity(entity->name());
                new_entity->set_transform(entity->transform());
                *value.selected = new_entity;
            }
            if (auto *selected_light = std::get_if<ufps::PointLightHandle>(value.selected))
            {
                const auto light = value.scene.lights().lights[*selected_light];
                ufps::ensure(!!light, "missing light?");

                *value.selected = value.scene.lights().lights.emplace(*light);
            }
        }
    }

    auto create_debug_control(const std::string &, LogView &) -> void
    {
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
    }

    // auto create_debug_control(const std::string &, Plot &value) -> void
    // {
    //     ::ImGui::PlotLines(
    //         "frame allocations",
    //         value.values.data(),
    //         value.values.size(),
    //         0,
    //         nullptr,
    //         0.0f,
    //         std::numeric_limits<float>::max(),
    //         ::ImVec2(0.f, 80.f));
    // }

    auto create_debug_control(const std::string &, SameLine &) -> void
    {
        ::ImGui::SameLine();
    }

    auto create_debug_control(const std::string &, TextureController &value) -> void
    {
        ::ImGui::Image(value.handle, ::ImVec2{value.width, value.height}, ::ImVec2(0.f, 1.f), ::ImVec2(1.f, 0.f));
    }

    template <class T>
    auto create_debug_controls(T &&data) -> void
    {
        const auto title = std::format("{}", clean_name(std::meta::display_string_of(std::meta::remove_cvref(^^T))));

        ::ImGui::PushID(title.c_str());

        ::ImGui::Text(title.c_str());

        constexpr auto ctx = std::meta::access_context::current();

        template for (constexpr auto &member : std::define_static_array(std::meta::nonstatic_data_members_of(std::meta::remove_cvref(^^T), ctx)))
        {
            const auto label = clean_name(std::meta::display_string_of(member));
            create_debug_control(label, data.[:member:]);
        }

        ::ImGui::PopID();
    }

    template <class... Controls>
    auto create_debug_window(const std::string &name, Controls &&...controls)
    {
        ::ImGui::Begin(name.c_str());

        (create_debug_controls(controls), ...);

        ::ImGui::End();
    }

}

namespace ufps
{
    DebugRenderer::DebugRenderer(const Window &window, ResourceLoader &resource_loader)
        : Renderer{window, resource_loader},
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
                                      { return create_aabb_lines(e.aabb(), selected_entity->transform(), {0.f, 0.2f, 0.f}); }) |
                std::views::join;

            _debug_lines.append_range(aabb_lines);
            _debug_lines.append_range(create_aabb_lines(selected_entity->aabb(), selected_entity->transform(), {0.f, 1.f, 0.f}));
        }

        Renderer::post_render(scene);

        if (!_enabled)
        {
            return;
        }

        auto &texture_manager = service<TextureManager>();

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

        const auto [vertex_buffer_handle, index_buffer_handle] = service<MeshManager>().native_handle();
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));
        ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_handle);

        const auto cube_parts = service<MeshManager>().mesh("cube");
        ensure(cube_parts.size() == 1u, "cube mesh should have exactly 1 part");
        const auto cube_indices_offset_bytes = cube_parts.front().index_offset * sizeof(std::uint32_t);
        const auto cube_vertex_offset = cube_parts.front().vertex_offset;

        for (const auto &light : scene.lights().lights.data())
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
        _debug_light_program.unbind();

        auto &&physics_debug_renderer = service<PhysicsSystem>().debug_renderer();
        if (physics_debug_renderer)
        {
            _debug_lines.append_range(physics_debug_renderer->yield_lines());
        }

        auto debug_line_count = 0zu;

        if (!_debug_lines.empty())
        {
            _debug_line_program.bind();
            debug_line_count = _debug_lines.size();

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

        struct BasicSceneInfo
        {
            float fps;
            float debug_lines;
            SaveSceneButton save_scene;
            AddLightButton add_light;
        };

        auto average_luminance = 0.0f;
        ::glGetNamedBufferSubData(_average_luminance_buffer.native_handle(), 0, sizeof(average_luminance), &average_luminance);

        std::uint32_t histogram[256]{};
        ::glGetNamedBufferSubData(_luminance_histogram_buffer.native_handle(), 0, sizeof(histogram), histogram);
        auto scaled_histogram = histogram |
                                std::views::transform([](const auto e)
                                                      { return std::log2(static_cast<float>(e) + 1.f); }) |
                                std::ranges::to<std::vector>();

        struct Luminance
        {
            float average_luminance;
            Histogram luminance;
        };

        struct SceneControls
        {
            AddEntity add_entity;
            DeleteEntity delete_entity;
            SameLine same_line{};
            DuplicateEntity duplicate_entity;
        };

        struct RemainingSceneInfo
        {
            Color &ambient;
            Matrix4 camera_view;
        };

        create_debug_window(
            "scene",
            BasicSceneInfo{
                .fps = io.Framerate,
                .debug_lines = static_cast<float>(debug_line_count),
                .save_scene = {.scene = scene},
                .add_light = {.scene = scene, .selected = &_selected},
            },
            scene.tone_map_options(),
            scene.ssao_options(),
            scene.bloom_options(),
            scene.fog_options(),
            scene.chromatic_abberation_options(),
            scene.vignette_options(),
            scene.film_grain_options(),
            scene.exposure_options(),
            Luminance{
                .average_luminance = average_luminance, .luminance = Histogram{.values = std::move(scaled_histogram)}},
            SceneControls{
                .add_entity = {.scene = scene, .selected = &_selected},
                .delete_entity = {.scene = scene, .selected = &_selected},
                .same_line = {},
                .duplicate_entity = {.scene = scene, .selected = &_selected}},
            RemainingSceneInfo{
                .ambient = scene.lights().ambient,
                .camera_view = scene.camera().data().view});

        struct LogWindow
        {
            LogView view;
        };

        create_debug_window("logs", LogWindow{});

        // static auto frame_allocations = Plot{.values = std::vector<float>(1000u)};
        // frame_allocations.values.erase(std::ranges::begin(frame_allocations.values));
        // frame_allocations.values.push_back(static_cast<float>(metrics().frame_allocated_bytes / 1024.f));
        // create_debug_window("metric", metrics(), Wrapper<Plot>{.controller = frame_allocations});

        static constexpr auto width = 175.f;
        const auto aspect_ratio = static_cast<float>(_window.width()) / static_cast<float>(_window.height());

        struct RenderTargets
        {
            TextureController gbuffer_color;
            SameLine same_line0;
            TextureController gbuffer_normals;
            SameLine same_line1;
            TextureController gbuffer_position;
            SameLine same_line2;
            TextureController gbuffer_specular;
            SameLine same_line3;
            TextureController gbuffer_roughness;
            TextureController gbuffer_ao;
            SameLine same_line4;
            TextureController gbuffer_emissive;
            SameLine same_line5;
            TextureController ssao;
            SameLine same_line6;
            TextureController transparancy;
            SameLine same_line7;
            TextureController bloom;
        };

        create_debug_window(
            "render_targets",
            RenderTargets{
                .gbuffer_color = {
                    texture_manager.texture(_gbuffer_rt.color_texture_bindless_handle_0)->native_handle(),
                    width * aspect_ratio,
                    width},
                .same_line0{},
                .gbuffer_normals = {texture_manager.texture(_gbuffer_rt.color_texture_bindless_handle_1)->native_handle(), width * aspect_ratio, width},
                .same_line1{},
                .gbuffer_position = {texture_manager.texture(_gbuffer_rt.color_texture_bindless_handle_2)->native_handle(), width * aspect_ratio, width},
                .same_line2{},
                .gbuffer_specular = {texture_manager.texture(_gbuffer_rt.color_texture_bindless_handle_3)->native_handle(), width * aspect_ratio, width},
                .same_line3{},
                .gbuffer_roughness = {texture_manager.texture(_gbuffer_rt.color_texture_bindless_handle_4)->native_handle(), width * aspect_ratio, width},
                .gbuffer_ao = {texture_manager.texture(_gbuffer_rt.color_texture_bindless_handle_5)->native_handle(), width * aspect_ratio, width},
                .same_line4{},
                .gbuffer_emissive = {texture_manager.texture(_gbuffer_rt.color_texture_bindless_handle_6)->native_handle(), width * aspect_ratio, width},
                .same_line5{},
                .ssao = {texture_manager.texture(_ssao_blur_rt.color_texture_bindless_handle_0)->native_handle(), width * aspect_ratio, width},
                .same_line6{},
                .transparancy = {texture_manager.texture(_forward_transparancy_rt.color_texture_bindless_handle_0)->native_handle(), width * aspect_ratio, width},
                .same_line7{},
                .bloom = {texture_manager.texture(_bloom_rt.color_texture_bindless_handle_0)->native_handle(), width * aspect_ratio, width},

            });

        ::ImGui::Begin("bloom_mip");
        for (const auto &[index, mip] : std::views::enumerate(_bloom_mips))
        {
            ::ImGui::Image(
                texture_manager.texture(mip.color_texture_bindless_handle_0)->native_handle(),
                ::ImVec2(width * aspect_ratio, width),
                ::ImVec2(0.f, 1.f),
                ::ImVec2(1.f, 0.f));

            if ((index + 1) % 4 != 0)
            {
                ::ImGui::SameLine();
            }
        }
        ::ImGui::End();

        if (!std::holds_alternative<std::monostate>(_selected))
        {
            ::ImGui::Begin("inspector");

            if (auto **selected_entity = std::get_if<Entity *>(&_selected))
            {
                auto *entity = *selected_entity;
                ::ImGui::Text("entity: %s", entity->name().c_str());

                if (::ImGui::Button("add rigid body"))
                {
                    const auto body = service<PhysicsSystem>().create_box({{-1.f}, {1.f}}, entity->position(), ufps::PhysicsLayer::DYNAMIC);
                    entity->add_rigid_body(body);
                }

                {
                    auto value = entity->emissive_strength();
                    if (::ImGui::SliderFloat("emissive_strength", &value, 0.f, 10.f))
                    {
                        entity->set_emissive_strength(value);
                    }
                }

                auto transform = Matrix4{entity->transform()};

                ::ImGui::BeginTable("transform", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit);

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

                auto debug_draw_texture = [scene, &texture_manager](auto idx, auto should_same_line) -> void
                {
                    if (idx < 65537)
                    {
                        const auto *texture = texture_manager.texture(idx);
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

                for (const auto &render_entity : entity->render_entities())
                {
                    debug_draw_texture(render_entity.albedo_texture_bindless_handle(), true);
                    debug_draw_texture(render_entity.normal_texture_bindless_handle(), true);
                    debug_draw_texture(render_entity.specular_texture_bindless_handle(), false);
                    debug_draw_texture(render_entity.roughness_texture_bindless_handle(), true);
                    debug_draw_texture(render_entity.ao_texture_bindless_handle(), true);
                    debug_draw_texture(render_entity.emissive_texture_bindless_handle(), false);
                }

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
            else if (auto *selected_handle = std::get_if<PointLightHandle>(&_selected))
            {
                auto light = scene.lights().lights[*selected_handle];
                ensure(!!light, "missing light ?");

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
            for (auto light_handle : scene.lights().lights.handles())
            {
                const auto light = scene.lights().lights[light_handle];
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