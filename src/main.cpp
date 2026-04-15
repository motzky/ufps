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

#include <yaml-cpp/yaml.h>

#include "config.h"
#include "core/render_entity.h"
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
#include "resources/resource_loader.h"
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

        auto factor = 96.f;
        if (is_key_pressed(ufps::Key::LSHIFT))
        {
            factor /= 4.f;
        }

        return direction / factor;
    }

    auto build_mesh_lookup(ufps::ResourceLoader &resource_loader) -> ufps::StringUnorderedMap<std::vector<ufps::MeshView>>
    {
        auto mesh_lookup = ufps::StringUnorderedMap<std::vector<ufps::MeshView>>{};

        const auto manifest_str = resource_loader.load_string("configs/model_manifest.yaml");
        const auto manifest = YAML::Load(manifest_str);

        for (const auto &model : manifest)
        {
            const auto &name = model.first.as<std::string>();
            const auto &sub_models = model.second;

            if (!sub_models.IsDefined() || !sub_models.IsMap())
            {
                ufps::log::warn("invalid manifest entry for model {}", model.first.as<std::string>());
                continue;
            }

            auto mesh_views = std::vector<ufps::MeshView>{};

            for (const auto &sub_model : sub_models)
            {
                const auto sub_model_name = sub_model.first.as<std::string>();
                const auto sub_model_data = sub_model.second;

                if (!sub_model_data || !sub_model_data["vertex_count"] || !sub_model_data["vertex_offset"] ||
                    !sub_model_data["index_count"] || !sub_model_data["index_offset"])
                {
                    ufps::log::warn("invalid manifest entry for model {}, submodel {}", name, sub_model_name);
                    continue;
                }

                const auto vertex_count = sub_model_data["vertex_count"].as<std::uint32_t>();
                const auto vertex_offset = sub_model_data["vertex_offset"].as<std::uint32_t>();
                const auto index_count = sub_model_data["index_count"].as<std::uint32_t>();
                const auto index_offset = sub_model_data["index_offset"].as<std::uint32_t>();

                mesh_views.push_back(
                    {.vertex_offset = vertex_offset,
                     .vertex_count = vertex_count,
                     .index_offset = index_offset,
                     .index_count = index_count});
            }

            mesh_lookup.insert({name, std::move(mesh_views)});
        }

        return mesh_lookup;
    }

    auto load_texture_from_blob(
        const ::YAML::Node &texture_manifest,
        const std::vector<std::byte> &texture_blob,
        const std::string &texture_name,
        ufps::ResourceLoader &resource_loader,
        ufps::TextureManager &texture_manager,
        const ufps::Sampler &sampler) -> std::uint32_t
    {
        if (!texture_manifest[texture_name] || !texture_manifest[texture_name]["offset"] || !texture_manifest[texture_name]["size"])
        {
            const auto texture_data = resource_loader.load_data_buffer(texture_name);
            auto texture = ufps::load_texture(texture_data, true);
            auto t = ufps::Texture{std::move(texture), texture_name, sampler};
            return texture_manager.add(std::move(t));
        }
        else
        {
            const auto offset = texture_manifest[texture_name]["offset"].as<std::uint64_t>();
            const auto size = texture_manifest[texture_name]["size"].as<std::uint64_t>();

            const auto texture_data = ufps::DataBufferView{
                texture_blob.data() + offset,
                size,
            };

            auto texture = ufps::load_texture(texture_data, true);
            auto t = ufps::Texture{std::move(texture), texture_name, sampler};
            return texture_manager.add(std::move(t));
        }
    }

    auto build_materials(
        ufps::ResourceLoader &resource_loader,
        ufps::TextureManager &texture_manager,
        ufps::MaterialManager &material_manager,
        const ufps::Sampler &sampler) -> ufps::StringUnorderedMap<std::vector<std::uint32_t>>
    {
        auto material_lookup = ufps::StringUnorderedMap<std::vector<std::uint32_t>>{};

        const auto model_manifest_str = resource_loader.load_string("configs/model_manifest.yaml");
        const auto model_manifest = ::YAML::Load(model_manifest_str);

        const auto texture_blob = ufps::decompress(resource_loader.load_data_buffer("blobs/texture_data.bin"));
        const auto texture_manifest_str = resource_loader.load_string("configs/texture_manifest.yaml");
        const auto texture_manifest = ::YAML::Load(texture_manifest_str);

        for (const auto &model : model_manifest)
        {
            const auto &name = model.first.as<std::string>();
            const auto &sub_models = model.second;

            if (!sub_models.IsDefined() || !sub_models.IsMap())
            {
                ufps::log::warn("invalid manifest entry for model {}", model.first.as<std::string>());
                continue;
            }

            for (const auto &sub_model : sub_models)
            {
                const auto sub_model_name = sub_model.first.as<std::string>();
                const auto sub_model_data = sub_model.second;

                if (!sub_model_data || !sub_model_data["vertex_count"] || !sub_model_data["vertex_offset"] ||
                    !sub_model_data["index_count"] || !sub_model_data["index_offset"])
                {
                    ufps::log::warn("invalid manifest entry for model {}, sub_model {}", name, sub_model_name);
                    continue;
                }

                const auto albedo_name = sub_model_data["albedo_name"].as<std::string>();
                const auto albedo_index = albedo_name.empty()
                                              ? 65567
                                              : *texture_manager.try_get_texture_index(albedo_name)
                                                     .or_else(
                                                         [&]
                                                         {
                                                             return std::make_optional(load_texture_from_blob(
                                                                 texture_manifest,
                                                                 texture_blob,
                                                                 albedo_name,
                                                                 resource_loader,
                                                                 texture_manager,
                                                                 sampler));
                                                         });

                const auto normal_name = sub_model_data["normal_name"].as<std::string>();
                const auto normal_index = normal_name.empty()
                                              ? 65567
                                              : *texture_manager.try_get_texture_index(normal_name)
                                                     .or_else(
                                                         [&]
                                                         {
                                                             return std::make_optional(load_texture_from_blob(
                                                                 texture_manifest,
                                                                 texture_blob,
                                                                 normal_name,
                                                                 resource_loader,
                                                                 texture_manager,
                                                                 sampler));
                                                         });

                const auto specular_name = sub_model_data["specular_name"].as<std::string>();
                const auto specular_index = specular_name.empty()
                                                ? 65567
                                                : *texture_manager.try_get_texture_index(specular_name)
                                                       .or_else(
                                                           [&]
                                                           {
                                                               return std::make_optional(load_texture_from_blob(
                                                                   texture_manifest,
                                                                   texture_blob,
                                                                   specular_name,
                                                                   resource_loader,
                                                                   texture_manager,
                                                                   sampler));
                                                           });

                const auto roughness_name = sub_model_data["roughness_name"].as<std::string>();
                const auto roughness_index = roughness_name.empty()
                                                 ? 65567
                                                 : *texture_manager.try_get_texture_index(roughness_name)
                                                        .or_else(
                                                            [&]
                                                            {
                                                                return std::make_optional(load_texture_from_blob(
                                                                    texture_manifest,
                                                                    texture_blob,
                                                                    roughness_name,
                                                                    resource_loader,
                                                                    texture_manager,
                                                                    sampler));
                                                            });

                const auto ao_name = sub_model_data["ambient_occlusion_name"].as<std::string>();
                const auto ao_index = ao_name.empty()
                                          ? 65567
                                          : *texture_manager.try_get_texture_index(ao_name)
                                                 .or_else(
                                                     [&]
                                                     {
                                                         return std::make_optional(load_texture_from_blob(
                                                             texture_manifest,
                                                             texture_blob,
                                                             ao_name,
                                                             resource_loader,
                                                             texture_manager,
                                                             sampler));
                                                     });

                const auto emissive_name = sub_model_data["emissive_color_name"].as<std::string>();
                const auto emissive_index = emissive_name.empty()
                                                ? 65567
                                                : *texture_manager.try_get_texture_index(emissive_name)
                                                       .or_else(
                                                           [&]
                                                           {
                                                               return std::make_optional(load_texture_from_blob(
                                                                   texture_manifest,
                                                                   texture_blob,
                                                                   emissive_name,
                                                                   resource_loader,
                                                                   texture_manager,
                                                                   sampler));
                                                           });

                const auto material_index = material_manager.add(ufps::Color{1.0f, 0.f, 1.f}, albedo_index, normal_index, specular_index, roughness_index, ao_index, emissive_index);
                material_lookup[name].push_back(material_index);
            }
        }
        return material_lookup;
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

            auto mesh_manager = ufps::MeshManager{
                ufps::decompress(resource_loader->load_data_buffer("blobs/vertex_data.bin")),
                ufps::decompress(resource_loader->load_data_buffer("blobs/index_data.bin")),
                build_mesh_lookup(*resource_loader)};
            auto material_manager = ufps::MaterialManager{};
            auto texture_manager = ufps::TextureManager{};

            mesh_manager.load("cube", std::vector{cube()});

            auto renderer = ufps::DebugRenderer{window, *resource_loader, texture_manager, mesh_manager};
            auto show_debug_ui = false;

            auto scene = ufps::Scene{
                mesh_manager,
                material_manager,
                texture_manager,
                {{},
                 {0.f, 0.f, -1.f},
                 {0.f, 1.f, 0.f},
                 std::numbers::pi_v<float> / 4.f,
                 static_cast<float>(window.width()),
                 static_cast<float>(window.height()),
                 0.01f,
                 1000.f},
                {
                    .ambient = {.r = .05f, .g = .05f, .b = .05f},
                    .lights = {
                        {.position = {0.f, 2.5f, 0.f},
                         .color = {.r = .5f, .g = .5f, .b = .5f},
                         .constant_attenuation = 1.f,
                         .linear_attenuation = .045f,
                         .quadratic_attenuation = .0075f,
                         .specular_power = 32.f,
                         .intensity = 1.f}},
                },
                {.max_brightness = 1.f, .contrast = 1.f, .linear_section_start = .22f, .linear_section_length = .4f, .black_tightness = 1.33f, .pedestal = 0.f, .gamma = 2.2f},
                {},
                {}};

            const auto material_lookup = build_materials(*resource_loader, texture_manager, material_manager, sampler);

            for (const auto &[name, materials] : material_lookup)
            {
                auto mesh_views = mesh_manager.mesh(name);
                auto render_entities = std::views::zip(mesh_views, materials) |
                                       std::views::transform(
                                           [&mesh_manager](const auto &e)
                                           {
                                               const auto &[mesh_view, material] = e;
                                               return ufps::RenderEntity(mesh_view, material, mesh_manager);
                                           }) |
                                       std::ranges::to<std::vector>();

                scene.cache_entity(name, {std::string{name}, std::move(render_entities), {}});
            }

            scene.create_entity("SM_Corner01_8_8_X");

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
                                if (!show_debug_ui || key_state[ufps::Key::LSHIFT])
                                {
                                    static constexpr auto sensitivity = float{0.002f};
                                    const auto delta_x = arg.delta_x() * sensitivity;
                                    const auto delta_y = arg.delta_y() * sensitivity;
                                    scene.camera().adjust_yaw(-delta_x);
                                    scene.camera().adjust_pitch(delta_y);
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

                scene.camera().translate(walk_direction(key_state, scene.camera()));
                scene.camera().update();

                renderer.render(scene);

                window.swap();
            }
        }
        catch (const ufps::Exception &err)
        {
            std::println(std::cerr, "{}", err);
        }
        catch (const std::runtime_error &err)
        {
            std::println(std::cerr, "{}", err.what());
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