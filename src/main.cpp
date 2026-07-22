#include <iostream>
#include <memory>
#include <numbers>
#include <print>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#ifndef WIN32
#include <GLFW/glfw3.h>
#endif

#include <yaml-cpp/yaml.h>

#include "concurrency/awaitable_manager.h"
#include "concurrency/task.h"
#include "concurrency/thread_pool.h"
#include "config.h"
#include "core/actor.h"
#include "core/flycam_actor.h"
#include "core/manifest_descriptions.h"
#include "core/player_actor.h"
#include "core/render_entity.h"
#include "core/scene.h"
#include "core/service_locator.h"
#include "events/input_map.h"
#include "graphics/command_buffer.h"
#include "graphics/debug_renderer.h"
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
#include "physics/physics_system.h"
#include "physics/rigid_body.h"
#include "resources/embedded_resource_loader.h"
#include "resources/file_resource_loader.h"
#include "resources/resource_loader.h"
#include "serialization/yaml_serializer.h"
#include "utils/data_buffer.h"
#include "utils/decompress.h"
#include "utils/ensure.h"
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

    [[maybe_unused]] auto load_all_textures(ufps::ResourceLoader &resource_loader, ufps::TextureManager &texture_manager, const ufps::Sampler &sampler) -> void
    {
        const auto texture_manifest_str = resource_loader.load_string("configs/texture_manifest.yaml");
        const auto texture_manifest = ufps::yaml::deserialize<ufps::TextureManifestDescription>(texture_manifest_str);
        ensure(texture_manifest);

        const auto texture_blob = ufps::decompress(resource_loader.load_data_buffer("blobs/texture_data.bin"));

        for (const auto &[name, manifest] : texture_manifest->textures)
        {
            const auto raw_texture_data = std::span{texture_blob.data() + manifest.offset, manifest.size};
            const auto texture_data = ufps::load_texture(raw_texture_data, manifest.is_srgb);

            texture_manager.add({texture_data, name, sampler});
        }
    }

    auto build_mesh_lookup(ufps::ResourceLoader &resource_loader) -> ufps::StringUnorderedMap<std::vector<ufps::MeshView>>
    {
        auto mesh_lookup = ufps::StringUnorderedMap<std::vector<ufps::MeshView>>{};

        const auto manifest_str = resource_loader.load_string("configs/model_manifest.yaml");
        const auto manifest = ufps::yaml::deserialize<ufps::ModelManifestDescription>(manifest_str);
        ensure(manifest);

        return manifest->models |
               std::views::transform([](const auto &e)
                                     {
            const auto &[name, manifests] = e;
            return std::pair{
                name,
                manifests |
                    std::views::transform([](const auto &m)
                                          { return m.mesh_view; }) |
                    std::ranges::to<std::vector>()}; }) |
               std::ranges::to<ufps::StringUnorderedMap<std::vector<ufps::MeshView>>>();
    }

    auto build_entity_cache(ufps::ResourceLoader &resource_loader) -> ufps::StringUnorderedMap<ufps::Entity>
    {
        auto entity_cache = ufps::StringUnorderedMap<ufps::Entity>{};

        const auto model_manifest_str = resource_loader.load_string("configs/model_manifest.yaml");
        const auto model_manifest = ufps::yaml::deserialize<ufps::ModelManifestDescription>(model_manifest_str);
        ensure(model_manifest);

        auto &texture_manager = ufps::service<ufps::TextureManager>();

        for (const auto &[name, manifests] : model_manifest->models)
        {
            auto render_entities = std::vector<ufps::RenderEntity>{};

            for (const auto &[mesh_view, albedo_name, normal_name, specular_name, roughness_name, ao_name, emissive_name, normal_compressed, opacity, emissive_intensity] : manifests)
            {

                const auto albedo_index = albedo_name.empty()
                                              ? 65567
                                              : texture_manager.bindless_handle(albedo_name);

                const auto normal_index = normal_name.empty()
                                              ? 65567
                                              : texture_manager.bindless_handle(normal_name);
                const auto specular_index = specular_name.empty()
                                                ? 65567
                                                : texture_manager.bindless_handle(specular_name);

                const auto roughness_index = roughness_name.empty()
                                                 ? 65567
                                                 : texture_manager.bindless_handle(roughness_name);

                const auto ao_index = ao_name.empty()
                                          ? 65567
                                          : texture_manager.bindless_handle(ao_name);

                const auto emissive_index = emissive_name.empty()
                                                ? 65567
                                                : texture_manager.bindless_handle(emissive_name);

                render_entities.push_back(
                    {mesh_view,
                     albedo_index,
                     normal_index,
                     specular_index,
                     roughness_index,
                     ao_index,
                     emissive_index,
                     normal_compressed,
                     opacity,
                     emissive_intensity});
            }
            entity_cache.insert({name, ufps::Entity{name, std::move(render_entities), {}}});
        }

        return entity_cache;
    }

    auto pulse_light(ufps::PointLightHandle handle, ufps::Scene &scene) -> ufps::Task
    {
        auto &awaitable = ufps::service<ufps::AwaitableManager>();
        auto fake_time = 0.f;

        for (;;)
        {
            if (auto light = scene.lights().lights[handle]; light)
            {
                light->intensity = 2.f + (5.f * ((std::sin(fake_time) + 1.f) / 2.f));
            }
            else
            {
                ufps::log::info("ending pulse_light coro");
                co_return;
            }

            fake_time += 0.01f;

            co_await awaitable;
        }
    }

    auto flicker_light(ufps::PointLightHandle handle, ufps::Scene &scene) -> ufps::Task
    {
        auto &awaitable = ufps::service<ufps::AwaitableManager>();
        auto light = scene.lights().lights[handle];
        if (!light)
        {
            ufps::log::warn("light did not exist when starting flicker_light coro");
            co_return;
        }

        auto original_intensity = light->intensity;
        for (;;)
        {
            if (auto light = scene.lights().lights[handle]; light)
            {
                co_await awaitable(3s);
                light->intensity = 0.f;
                co_await awaitable(100ms);
                light->intensity = original_intensity;
            }
            else
            {
                ufps::log::info("ending flicker_light coro");
                co_return;
            }
        }
    }
}

auto start(int argc, char **argv) -> int
{
    if (ufps::version::tweak == 0)
    {
        ufps::log::info("starting ufps {}.{}.{}", ufps::version::year, ufps::version::month, ufps::version::day);
    }
    else
    {
        ufps::log::info("starting ufps {}.{}.{}.{}", ufps::version::year, ufps::version::month, ufps::version::day, ufps::version::tweak);
    }

    const auto args = std::vector<std::string_view>(argv + 1u, argv + argc);

    ufps::log::info("args: {}", args);

    auto window = ufps::Window{1920u, 1080u, 0u, 0u};
    [[maybe_unused]] auto running = true;

    const auto sampler = ufps::Sampler{
        ufps::FilterType::LINEAR,
        ufps::FilterType::LINEAR,
        ufps::WrapMode::REPEAT,
        ufps::WrapMode::REPEAT,
        "sampler"};

    auto resource_loader = std::unique_ptr<ufps::ResourceLoader>();
    if constexpr (ufps::config::use_embedded_resource_loader)
    {
        ufps::log::info("using embedded resource loader");
        resource_loader = std::make_unique<ufps::EmbeddedResourceLoader>();
    }
    else
    {
        ufps::log::info("using file resource loader");
        resource_loader = std::make_unique<ufps::FileResourceLoader>(std::vector<std::filesystem::path>{"assets", "build/build_assets"});
    }

    auto texture_manager = std::make_unique<ufps::TextureManager>();
    load_all_textures(*resource_loader, *texture_manager, sampler);

    auto pool = std::make_unique<ufps::ThreadPool>();
    auto awaitable_manager = std::make_unique<ufps::AwaitableManager>(*pool);

    auto mesh_manager = std::make_unique<ufps::MeshManager>(
        ufps::decompress(resource_loader->load_data_buffer("blobs/vertex_data.bin")),
        ufps::decompress(resource_loader->load_data_buffer("blobs/index_data.bin")),
        build_mesh_lookup(*resource_loader));

    mesh_manager->load("cube", std::vector{cube()});

    auto physics = std::make_unique<ufps::PhysicsSystem>(ufps::DebugRenderMode::ON);
    auto &player_controller = physics->player_controller();

    auto input_map = ufps::InputMap{};

    auto player_actor = ufps::PlayerActor{
        {{0.f, 2.f, 0.f},
         {0.f, 0.f, -1.f},
         {0.f, 1.f, 0.f},
         std::numbers::pi_v<float> / 4.f,
         static_cast<float>(window.width()),
         static_cast<float>(window.height()),
         0.01f,
         1000.f},
        input_map,
        player_controller};

    auto flycam_actor = ufps::FlyCamActor{
        {{0.f, 2.f, 0.f},
         {0.f, 0.f, -1.f},
         {0.f, 1.f, 0.f},
         std::numbers::pi_v<float> / 4.f,
         static_cast<float>(window.width()),
         static_cast<float>(window.height()),
         0.01f,
         1000.f},
        input_map};

    ufps::Actor *current_actor = std::addressof(player_actor);

    auto ss = std::stringstream{};
    auto scene_description_yaml = std::ifstream{"scene.yaml"};

    if (scene_description_yaml.is_open())
    {
        ss << scene_description_yaml.rdbuf();
    }
    else
    {
        if constexpr (ufps::config::use_embedded_resource_loader)
        {
            auto scene_description_str = resource_loader->load_string("configs/scene.yaml");
            ss << scene_description_str;
        }
    }

    auto services = std::make_unique<ufps::Services>(
        std::move(awaitable_manager),
        std::move(mesh_manager),
        std::move(physics),
        std::move(texture_manager),
        std::move(pool));
    ufps::set_services(services.get());

    auto renderer = ufps::DebugRenderer{window, *resource_loader};
    auto show_debug_ui = false;

    auto scene_desc = ufps::yaml::deserialize<ufps::Scene::Description>(ss.str());
    ufps::ensure(scene_desc);

    auto scene = ufps::Scene{
        std::move(*scene_desc),
        build_entity_cache(*resource_loader)};

    const auto point_light_handles = scene.lights().lights.handles();
    pulse_light(point_light_handles[0], scene);
    flicker_light(point_light_handles[1], scene);

    while (running)
    {
        auto &physics = ufps::service<ufps::PhysicsSystem>();
        auto &awaitable = ufps::service<ufps::AwaitableManager>();
        auto &pool = ufps::service<ufps::ThreadPool>();

        input_map.delta_x = 0.f;
        input_map.delta_y = 0.f;

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

                        if (input_map[ufps::Key::ESC])
                        {
                            if (show_debug_ui)
                            {
                                ufps::log::info("hiding Debug UI");
                                show_debug_ui = false;
                                renderer.set_enabled(show_debug_ui);
                            }
                            else
                            {
                                ufps::log::info("stopping");
                                running = false;
                            }
                        }
                        // else if (arg.key() == ufps::Key::F1 && arg.state() == ufps::KeyState::UP)
                        else if (input_map[ufps::Key::F1])
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
                            current_actor = show_debug_ui ? static_cast<ufps::Actor *>(std::addressof(flycam_actor)) : std::addressof(player_actor);
                        }

                        input_map.set(arg);
                    }
                    else if constexpr (std::same_as<T, ufps::MouseEvent>)
                    {
                        if (!show_debug_ui || input_map[ufps::Key::LSHIFT])
                        {
                            static constexpr auto sensitivity = float{0.002f};
                            input_map.delta_x += arg.delta_x() * sensitivity;
                            input_map.delta_y += arg.delta_y() * sensitivity;
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

        current_actor->update();

        physics.update();
        awaitable.pump();
        pool.drain();

        renderer.render(scene, current_actor->camera());

        window.swap();
    }

    ufps::service<ufps::AwaitableManager>().pump();
    ufps::service<ufps::ThreadPool>().drain();

    services.release();

    return 0;
}

auto main(int argc, char **argv) -> int
{
    auto return_code = 0;
    try
    {
        return_code = start(argc, argv);
    }
    catch (const ufps::Exception &err)
    {
        ufps::log::error("{}", err);
        return_code = -1;
    }
    catch (const std::exception &e)
    {
        ufps::log::error("{}", e.what());
        return_code = -1;
    }
    catch (...)
    {
        ufps::log::error("unhadnled unknown exception");
        return_code = -1;
    }

#ifndef WIN32
    ::glfwTerminate();
#endif

    return return_code;
}