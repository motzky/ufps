#include <iostream>
#include <memory>
#include <numbers>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#ifndef WIN32
#include <GLFW/glfw3.h>
#endif

#include "config.h"
#include "core/scene.h"
#include "graphics/command_buffer.h"
#include "graphics/debug_renderer.h"
#include "graphics/material_manager.h"
#include "graphics/mesh_data.h"
#include "graphics/mesh_manager.h"
#include "graphics/multi_buffer.h"
#include "graphics/persistent_buffer.h"
#include "graphics/renderer.h"
#include "graphics/sampler.h"
#include "graphics/texture.h"
#include "graphics/texture_manager.h"
#include "graphics/utils.h"
#include "graphics/vertex_data.h"
#include "log.h"
#include "resources/embedded_resource_loader.h"
#include "resources/file_resource_loader.h"
#include "utils/data_buffer.h"
#include "utils/exception.h"
#include "window.h"

using namespace std::literals;

namespace
{
    [[maybe_unused]] auto cube() -> ufps::MeshData
    {
        const ufps::Vector3 positions[] = {
            {-1.f, -1.f, 1.f},
            {1.f, -1.f, 1.f},
            {1.f, 1.f, 1.f},
            {-1.f, 1.f, 1.f},
            {1.f, -1.f, 1.f},
            {1.f, -1.f, -1.f},
            {1.f, 1.f, -1.f},
            {1.f, 1.f, 1.f},
            {1.f, -1.f, -1.f},
            {-1.f, -1.f, -1.f},
            {-1.f, 1.f, -1.f},
            {1.f, 1.f, -1.f},
            {-1.f, -1.f, -1.f},
            {-1.f, -1.f, 1.f},
            {-1.f, 1.f, 1.f},
            {-1.f, 1.f, -1.f},
            {-1.f, 1.f, 1.f},
            {1.f, 1.f, 1.f},
            {1.f, 1.f, -1.f},
            {-1.f, 1.f, -1.f},
            {-1.f, -1.f, -1.f},
            {1.f, -1.f, -1.f},
            {1.f, -1.f, 1.f},
            {-1.f, -1.f, 1.f}};

        const ufps::Vector3 normals[] = {
            {0.f, 0.f, 1.f},
            {0.f, 0.f, 1.f},
            {0.f, 0.f, 1.f},
            {0.f, 0.f, 1.f},
            {1.f, 0.f, 0.f},
            {1.f, 0.f, 0.f},
            {1.f, 0.f, 0.f},
            {1.f, 0.f, 0.f},
            {0.f, 0.f, -1.f},
            {0.f, 0.f, -1.f},
            {0.f, 0.f, -1.f},
            {0.f, 0.f, -1.f},
            {-1.f, 0.f, 0.f},
            {-1.f, 0.f, 0.f},
            {-1.f, 0.f, 0.f},
            {-1.f, 0.f, 0.f},
            {0.f, 1.f, 0.f},
            {0.f, 1.f, 0.f},
            {0.f, 1.f, 0.f},
            {0.f, 1.f, 0.f},
            {0.f, -1.f, 0.f},
            {0.f, -1.f, 0.f},
            {0.f, -1.f, 0.f},
            {0.f, -1.f, 0.f},
        };

        const ufps::UV uvs[] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f},
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f},
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f},
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f},
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f},
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f},
        };

        auto indices = std::vector<std::uint32_t>{
            // Each face: 2 triangles (6 indices)
            // Front face
            0, 1, 2, 2, 3, 0,
            // Right face
            4, 5, 6, 6, 7, 4,
            // Back face
            8, 9, 10, 10, 11, 8,
            // Left face
            12, 13, 14, 14, 15, 12,
            // Top face
            16, 17, 18, 18, 19, 16,
            // Bottom face
            20, 21, 22, 22, 23, 20};

        auto vs = ufps::MeshData{.vertices = vertices(positions, normals, normals, normals, uvs),
                                 .indices = std::move(indices)};

        for (const auto &indices : std::views::chunk(vs.indices, 3))
        {
            auto &v0 = vs.vertices[indices[0]];
            auto &v1 = vs.vertices[indices[1]];
            auto &v2 = vs.vertices[indices[2]];

            const auto edge1 = v1.position - v0.position;
            const auto edge2 = v2.position - v0.position;

            const auto deltaUV1 = ufps::UV{.s = v1.uv.s - v0.uv.s, .t = v1.uv.t - v0.uv.t};
            const auto deltaUV2 = ufps::UV{.s = v2.uv.s - v0.uv.s, .t = v2.uv.t - v0.uv.t};

            const auto f = 1.0f / (deltaUV1.s * deltaUV2.t - deltaUV2.s * deltaUV1.t);

            const auto tangent = ufps::Vector3{
                f * (deltaUV2.t * edge1.x - deltaUV1.t * edge2.x),
                f * (deltaUV2.t * edge1.y - deltaUV1.t * edge2.y),
                f * (deltaUV2.t * edge1.z - deltaUV1.t * edge2.z),
            };

            v0.tangent += tangent;
            v1.tangent += tangent;
            v2.tangent += tangent;
        }

        for (auto &v : vs.vertices)
        {
            v.tangent = ufps::Vector3::normalize(v.tangent - v.normal * ufps::Vector3::dot(v.normal, v.tangent));
            v.bitangent = ufps::Vector3::normalize(ufps::Vector3::cross(v.normal, v.tangent));
        }

        return vs;
    }

    auto walk_direction(const std::unordered_map<ufps::Key, bool> &key_state, const ufps::Camera &camera) -> ufps::Vector3
    {
        auto direction = ufps::Vector3{};

        auto is_key_pressed = [&key_state](ufps::Key k) -> bool
        {
            auto e = key_state.find(k);
            return (e != key_state.end()) && e->second;
        };

        if (is_key_pressed(ufps::Key::W))
        {
            direction += camera.direction();
        }
        if (is_key_pressed(ufps::Key::S))
        {
            direction -= camera.direction();
        }
        if (is_key_pressed(ufps::Key::D))
        {
            direction += camera.right();
        }
        if (is_key_pressed(ufps::Key::A))
        {
            direction -= camera.right();
        }
        if (is_key_pressed(ufps::Key::SPACE))
        {
            direction += camera.up();
        }
        if (is_key_pressed(ufps::Key::LCTRL))
        {
            direction -= camera.up();
        }

        return direction / 4.0f;
    }
}

auto main(int argc, char **argv) -> int
{
    ufps::log::info("starting game {}.{}.{}", ufps::version::major, ufps::version::minor, ufps::version::patch);

    const auto args = std::vector<std::string_view>(argv + 1u, argv + argc);

#if defined(_MSC_VER) || defined(__GNUC__) && (__GNUC__ >= 15)
    // only msvc and GCC >=15 support automatic formatting of std::vector
    ufps::log::info("args: {}", args);
#else
    std::print("args: [");
    for (const auto &s : args)
    {
        std::print("\"{}\", ", s);
    }
    std::println("]");
#endif

    {

        try
        {
            auto window = ufps::Window{1920u, 1080u, 0u, 0u};
            auto running = true;

            const auto sampler = ufps::Sampler{ufps::FilterType::LINEAR, ufps::FilterType::LINEAR, "sampler"};

            std::unique_ptr<ufps::ResourceLoader> resource_loader = std::make_unique<ufps::EmbeddedResourceLoader>();
            auto textures = std::vector<ufps::Texture>{};

            const auto diamond_floor_albedo_data = resource_loader->load_data_buffer("textures/diamond_floor_albedo.png");
            const auto diamond_floor_albedo = ufps::load_texture(diamond_floor_albedo_data);
            textures.push_back(ufps::Texture{diamond_floor_albedo, "diamond_floor_albedo", sampler});

            const auto diamond_floor_normal_data = resource_loader->load_data_buffer("textures/diamond_floor_normal.png");
            const auto diamond_floor_normal = ufps::load_texture(diamond_floor_normal_data);
            textures.push_back(ufps::Texture{diamond_floor_normal, "diamond_floor_normal", sampler});

            const auto diamond_floor_specular_data = resource_loader->load_data_buffer("textures/diamond_floor_specular.png");
            const auto diamond_floor_specular = ufps::load_texture(diamond_floor_specular_data);
            textures.push_back(ufps::Texture{diamond_floor_specular, "diamond_floor_specular", sampler});

            auto mesh_manager = ufps::MeshManager{};
            auto material_manager = ufps::MaterialManager{};
            auto texture_manager = ufps::TextureManager{};

            const auto tex_index = texture_manager.add(std::move(textures));

            auto renderer = ufps::DebugRenderer{window, *resource_loader, texture_manager, mesh_manager};
            auto show_debug_ui = false;

            [[maybe_unused]] const auto material_index_r = material_manager.add(ufps::Color{1.0f, 0.f, 0.f}, tex_index, tex_index + 1u, tex_index + 2u);
            [[maybe_unused]] const auto material_index_g = material_manager.add(ufps::Color{0.0f, 1.f, 0.f}, tex_index, tex_index + 1u, tex_index + 2u);
            [[maybe_unused]] const auto material_index_b = material_manager.add(ufps::Color{0.0f, 0.f, 1.f}, tex_index, tex_index + 1u, tex_index + 2u);

            const auto models = ufps::load_model(resource_loader->load_data_buffer("models/SM_Corner01_8_8_X.fbx"), "fbx");

            auto scene = ufps::Scene{
                .entities = {},
                .mesh_manager = mesh_manager,
                .material_manager = material_manager,
                .texture_manager = texture_manager,
                .camera = {
                    {},
                    {0.f, 0.f, -1.f},
                    {0.f, 1.f, 0.f},
                    std::numbers::pi_v<float> / 4.f,
                    static_cast<float>(window.width()),
                    static_cast<float>(window.height()),
                    0.01f,
                    1000.f},
                .lights = {
                    .ambient = {.r = .15f, .g = .15f, .b = .15f},
                    .light = {
                        .position = {},
                        .color = {.r = 1.f, .g = 1.f, .b = 1.f},
                        .constant_attenuation = 1.f,
                        .linear_attenuation = .045f,
                        .quadratic_attenuation = .0075f,
                        .specular_poewr = 32.f,
                    },
                }};

            scene.entities = models |
                             std::views::enumerate |
                             std::views::transform([&](const auto &e)
                                                   {
                                                        const auto &[index, model] = e;
                                                        return ufps::Entity{
                                                            .name = std::format("SM_Corner01_8_8_X_{}", index),
                                                            .mesh_view = mesh_manager.load(model.mesh_data),
                                                            .transform = {{}, {1.f}, {}},
                                                            .material_index = material_index_r}; }) |
                             std::ranges::to<std::vector>();

            auto key_state = std::unordered_map<ufps::Key, bool>{
                {ufps::Key::A, false},
                {ufps::Key::D, false},
                {ufps::Key::S, false},
                {ufps::Key::W, false},
            };

            while (running)
            {
                auto event = window.pump_event();
                while (event && running)
                {
                    std::visit(
                        [&](auto &&arg)
                        {
                            using T = std::decay_t<decltype(arg)>;

                            if constexpr (std::same_as<T, ufps::StopEvent>)
                            {
                                ufps::log::info("stopping");
                                running = false;
                            }
                            if constexpr (std::same_as<T, ufps::KeyEvent>)
                            {
                                if (arg.key() == ufps::Key::ESC && arg.state() == ufps::KeyState::UP)
                                {
                                    ufps::log::info("stopping");
                                    running = false;
                                }
                                else if (arg.key() == ufps::Key::F1 && arg.state() == ufps::KeyState::UP)
                                {
                                    if (!show_debug_ui)
                                    {
                                        ufps::log::info("showing Debug UI");
                                    }
                                    else
                                    {
                                        ufps::log::info("hiding Debug UI");
                                    }
                                    show_debug_ui = !show_debug_ui;
                                    renderer.set_enabled(show_debug_ui);
                                }
                                else
                                {
                                    key_state[arg.key()] = arg.state() == ufps::KeyState::DOWN;
                                }
                            }
                            else if constexpr (std::same_as<T, ufps::MouseEvent>)
                            {
                                if (!show_debug_ui)
                                {
                                    static constexpr auto sensitivity = float{0.002f};
                                    const auto delta_x = arg.delta_x() * sensitivity;
                                    const auto delta_y = arg.delta_y() * sensitivity;
                                    scene.camera.adjust_yaw(-delta_x);
                                    scene.camera.adjust_pitch(delta_y);
                                }
                            }
                            else if constexpr (std::same_as<T, ufps::MouseButtonEvent>)
                            {
                                if (show_debug_ui)
                                {
                                    renderer.add_mouse_event(arg);
                                }
                            }
                        },
                        *event);

                    event = window.pump_event();
                }

                scene.camera.translate(walk_direction(key_state, scene.camera));
                scene.camera.update();

                renderer.render(scene);

                window.swap();
            }
        }
        catch (const ufps::Exception &err)
        {
            std::println(std::cerr, "{}", err);
        }
        catch (...)
        {
            std::println(std::cerr, "unknown exception");
        }
    }

#ifndef WIN32
    ::glfwTerminate();
#endif

    return 0;
}