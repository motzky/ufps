#include "graphics/utils.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <ranges>
#include <tuple>
#include <vector>

#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/Logger.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma GCC diagnostic pop

#include "graphics/dds.h"
#include "graphics/model_data.h"
#include "graphics/texture_data.h"
#include "graphics/vertex_data.h"
#include "log.h"
#include "resources/resource_loader.h"
#include "utils/data_buffer.h"
#include "utils/ensure.h"
#include "utils/formatter.h"
#include "utils/string_unordered_map.h"

namespace
{

    template <ufps::log::Level L>
    class AssimpLogStreamAdapter : public ::Assimp::LogStream
    {
    public:
        auto write(const char *msg) -> void override
        {
            auto s = std::string(msg);
            if (s.ends_with("\n"))
            {
                s = s.substr(0, s.length() - 1);
            }
            auto idx = s.find(',');
            if (idx > 0 && idx < 10)
            {
                idx = s.find("T", idx);
                s = s.substr(idx);
            }
            ufps::log::Print("{}", s);
        }
    };

    auto to_native(const ::aiVector3D &v) -> ufps::Vector3
    {
        return {v.x, v.y, v.z};
    }

    auto channels_to_format(int num_channels, bool is_srgb) -> ufps::TextureFormat
    {
        switch (num_channels)
        {
            using enum ufps::TextureFormat;
        case 1:
            return R;
        case 3:
            return is_srgb ? SRGB : RGB;
        case 4:
            return is_srgb ? SRGBA : RGBA;
        default:
            throw ufps::Exception("unsupported channel count: {}", num_channels);
        }
    }

    [[maybe_unused]] auto to_string(::aiTextureType type) -> std::string
    {
        const auto str = std::string{::aiTextureTypeToString(type)};
        return str;
    }

    auto get_texture_file_name(const ::aiMaterial *material, ::aiTextureType type) -> std::optional<std::string>
    {
        if (material->GetTextureCount(type) == 0)
        {
            // ufps::log::warn("no texture of type {} found", to_string(type));
            return std::nullopt;
        }

        auto path_str = ::aiString{};
        material->GetTexture(type, 0, &path_str);
        auto str = std::string{path_str.C_Str()};
        std::replace(str.begin(), str.end(), '\\', '/');
        const auto path = std::filesystem::path{str};
        const auto filename = path.filename();
        // ufps::log::debug("found texture: {} type: {}", filename.string(), to_string(type));

        if (filename.empty())
        {
            return std::nullopt;
        }
        return std::format("textures/{}", filename.string());
    }

    auto is_texture_type_implmented(::aiTextureType type) -> bool
    {
        switch (type)
        {
        case ::aiTextureType_BASE_COLOR:
        case ::aiTextureType_DIFFUSE:
        case ::aiTextureType_NORMAL_CAMERA:
        case ::aiTextureType_METALNESS:
        case ::aiTextureType_DIFFUSE_ROUGHNESS:
        case ::aiTextureType_AMBIENT_OCCLUSION:
        case ::aiTextureType_EMISSION_COLOR:
            return true;
        default:
            return false;
        }
    }

    auto dump_texture_types_of_material(const ::aiMaterial *const material, bool ignore_handled = true) -> void
    {
        for (int i = ::aiTextureType_NONE; i < ::aiTextureType_GLTF_METALLIC_ROUGHNESS; ++i)
        {
            auto type = static_cast<::aiTextureType>(i);
            if (ignore_handled && is_texture_type_implmented(type))
            {
                continue;
            }
            const auto cnt = material->GetTextureCount(type);
            if (cnt == 0)
            {
                continue;
            }
            if (ignore_handled)
            {
                ufps::log::warn("   UNHANDLED Texture type: {}, count: {}", ::aiTextureTypeToString(type), cnt);
                continue;
            }
            ufps::log::debug("   Texture type: {}, count: {}", ::aiTextureTypeToString(type), cnt);
        }
    }

}

namespace ufps
{
    static StringUnorderedMap<TextureData> _texture_cache;

    auto load_texture(ResourceLoader &resource_loader, std::string id, bool is_srgb) -> TextureData
    {
        auto it = _texture_cache.find(id);
        if (it == _texture_cache.end())
        {
            // log::debug("loading texture {}", id);
            auto tex = load_texture(resource_loader.load_data_buffer(id), is_srgb);
            _texture_cache.insert(std::make_pair(id, tex));
            return tex;
        }

        return it->second;
    }

    auto load_texture(DataBufferView image_data, bool is_srgb) -> TextureData
    {
        auto width = int{};
        auto height = int{};
        auto num_channels = int{};

        static constexpr std::byte dds_magic[] = {
            static_cast<std::byte>(0x44),
            static_cast<std::byte>(0x44),
            static_cast<std::byte>(0x53),
            static_cast<std::byte>(0x20),
        };

        if (std::ranges::equal(dds_magic, image_data | std::views::take(sizeof(dds_magic))))
        {
            log::debug("found DDS");

            auto dds_header = DDS_HEADER{};
            std::memcpy(&dds_header, image_data.data() + sizeof(dds_magic), sizeof(dds_header));

            ensure(dds_header.dwSize == sizeof(dds_header), "invalid dds_header size: {}", dds_header.dwSize);
            ensure(dds_header.ddspf.dwFourCC == 0x30315833, "not DX10 format");

            auto dx10_header = DDS_HEADER_DXT10{};
            std::memcpy(&dx10_header, image_data.data() + sizeof(dds_magic) + sizeof(dds_header), sizeof(dx10_header));

            return {
                .width = static_cast<std::uint32_t>(dds_header.dwWidth),
                .height = static_cast<std::uint32_t>(dds_header.dwHeight),
                .format = TextureFormat::BC7, // should be read from dds
                .data = image_data |
                        std::views::drop(sizeof(dds_magic) + sizeof(dds_header) + sizeof(dx10_header)) |
                        std::ranges::to<std::vector>(),
                .is_compressed = true,
            };
        }
        else
        {
            log::debug("found non-DDS image");

            auto raw_data = std::unique_ptr<::stbi_uc, void (*)(void *)>{
                ::stbi_load_from_memory(
                    reinterpret_cast<const ::stbi_uc *>(image_data.data()),
                    image_data.size(),
                    &width,
                    &height,
                    &num_channels,
                    0),
                ::stbi_image_free};

            ensure(raw_data, "failed to parse texture data");

            const auto *ptr = reinterpret_cast<const std::byte *>(raw_data.get());

            // log::debug("  num_channels: {}", num_channels);

            return {
                .width = static_cast<std::uint32_t>(width),
                .height = static_cast<std::uint32_t>(height),
                .format = channels_to_format(num_channels, is_srgb),
                .data = {{ptr, ptr + width * height * num_channels}},
                .is_compressed = false,
            };
        }
    }

    auto load_model(DataBufferView model_data, ResourceLoader &resource_loader, std::string format) -> std::tuple<std::string, std::vector<ModelData>>
    {
        if (config::log_assimp)
        {
            [[maybe_unused]] static auto *logger = []()
            {
                ::Assimp::DefaultLogger::create("", ::Assimp::Logger::VERBOSE);
                auto *logger = ::Assimp::DefaultLogger::get();

                logger->attachStream(new AssimpLogStreamAdapter<ufps::log::Level::ERROR>{}, ::Assimp::Logger::Err);
                logger->attachStream(new AssimpLogStreamAdapter<ufps::log::Level::WARN>{}, ::Assimp::Logger::Warn);
                logger->attachStream(new AssimpLogStreamAdapter<ufps::log::Level::INFO>{}, ::Assimp::Logger::Info);
                logger->attachStream(new AssimpLogStreamAdapter<ufps::log::Level::DEBUG>{}, ::Assimp::Logger::Debugging);

                return logger;
            }();
        }

        auto importer = ::Assimp::Importer{};
        auto *scene = importer.ReadFileFromMemory(
            model_data.data(),
            model_data.size(),
            ::aiPostProcessSteps::aiProcess_Triangulate | ::aiPostProcessSteps::aiProcess_FlipUVs | ::aiPostProcessSteps::aiProcess_CalcTangentSpace,
            format.c_str());

        ensure(scene != nullptr, "failed to parse assimp scene");

        const auto loaded_meshes = std::span<::aiMesh *>(scene->mMeshes, scene->mMeshes + scene->mNumMeshes);
        const auto materials = std::span<::aiMaterial *>(scene->mMaterials, scene->mMaterials + scene->mNumMaterials);

        if (config::log_assimp)
        {
            log::info("found {} meshes, {} materials {} lights", loaded_meshes.size(), materials.size(), scene->mNumLights);
        }

        ensure(loaded_meshes.size() == materials.size(), "mismatch mesh/material count in model file");

        auto models = std::vector<ModelData>{};

        for (const auto &[index, mesh] : loaded_meshes | std::views::enumerate)
        {
            const auto *material = scene->mMaterials[index];
            log::info("found mesh: {}, material: {}", mesh->mName.C_Str(), material->GetName().C_Str());

            const auto base_color_count = material->GetTextureCount(::aiTextureType_BASE_COLOR);
            const auto diffuse_color_count = material->GetTextureCount(::aiTextureType_DIFFUSE);
            if (base_color_count != 1 && diffuse_color_count == 0)
            {
                log::warn("unsupported base color count: {}", base_color_count);
                continue;
            }

            if (config::log_assimp)
            {
                dump_texture_types_of_material(material, false);
            }

            const auto albedo_filename =
                diffuse_color_count > 0 ? get_texture_file_name(material, ::aiTextureType_DIFFUSE) : get_texture_file_name(material, ::aiTextureType_BASE_COLOR);

            const auto normal_filename = get_texture_file_name(material, ::aiTextureType_NORMAL_CAMERA);
            const auto specular_filename = get_texture_file_name(material, ::aiTextureType_METALNESS);
            const auto roughness_filename = get_texture_file_name(material, ::aiTextureType_DIFFUSE_ROUGHNESS);
            const auto ao_filename = get_texture_file_name(material, ::aiTextureType_AMBIENT_OCCLUSION);
            const auto emissive_filename = get_texture_file_name(material, ::aiTextureType_EMISSION_COLOR);

            auto opacity = 1.0f;
            if (material->Get(AI_MATKEY_OPACITY, opacity) != AI_SUCCESS)
            {
                opacity = 1.f;
            }

            if (opacity < 1.f)
            {
                log::debug("Opacity: {}", opacity);
                auto refracti = 1.f;
                if (material->Get(AI_MATKEY_REFRACTI, refracti) == AI_SUCCESS)
                {
                    log::debug("Refract idx: {}", refracti);
                }
            }

            const auto positions = std::span<::aiVector3D>{mesh->mVertices, mesh->mVertices + mesh->mNumVertices} | std::views::transform(to_native);
            const auto normals = std::span<::aiVector3D>{mesh->mNormals, mesh->mNormals + mesh->mNumVertices} | std::views::transform(to_native);
            const auto tangents = std::span<::aiVector3D>{mesh->mTangents, mesh->mTangents + mesh->mNumVertices} | std::views::transform(to_native);
            const auto bitangents = std::span<::aiVector3D>{mesh->mBitangents, mesh->mBitangents + mesh->mNumVertices} | std::views::transform(to_native);

            const auto uvs =
                std::span<::aiVector3D>{mesh->mTextureCoords[0], mesh->mTextureCoords[0] + mesh->mNumVertices} |
                std::views::transform([](const auto &v)
                                      { return UV{.s = v.x, .t = v.y}; });

            auto indices =
                std::span<::aiFace>{mesh->mFaces, mesh->mFaces + mesh->mNumFaces} |
                std::views::transform([](const auto &f)
                                      { return std::span<std::uint32_t>{f.mIndices, f.mIndices + f.mNumIndices}; }) |
                std::views::join |
                std::ranges::to<std::vector>();

            auto model = ModelData{
                .mesh_data = {.vertices = vertices(positions, normals, tangents, bitangents, uvs),
                              .indices = std::move(indices)},
                .albedo = std::nullopt,
                .normal = std::nullopt,
                .specular = std::nullopt,
                .roughness = std::nullopt,
                .ambient_occlusion = std::nullopt,
                .emissive_color = std::nullopt,
                .opacity = opacity};

            if (albedo_filename.has_value())
            {
                model.albedo = load_texture(resource_loader, *albedo_filename, true);
            }
            if (normal_filename.has_value())
            {
                model.normal = load_texture(resource_loader, *normal_filename, false);
            }
            if (specular_filename.has_value())
            {
                model.specular = load_texture(resource_loader, *specular_filename, false);
            }
            if (roughness_filename.has_value())
            {
                model.roughness = load_texture(resource_loader, *roughness_filename, false);
            }
            if (ao_filename.has_value())
            {
                model.ambient_occlusion = load_texture(resource_loader, *ao_filename, false);
            }
            if (emissive_filename.has_value())
            {
                model.emissive_color = load_texture(resource_loader, *emissive_filename, true);
            }

            models.push_back(model);
        }

        return {loaded_meshes.front()->mName.C_Str(), models};
    }
}