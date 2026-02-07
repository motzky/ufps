#include "graphics/utils.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <optional>
#include <ranges>
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

#include "graphics/model_data.h"
#include "graphics/texture_data.h"
#include "graphics/vertex_data.h"
#include "log.h"
#include "resources/resource_loader.h"
#include "utils/data_buffer.h"
#include "utils/ensure.h"
#include "utils/formatter.h"

namespace
{
    template <ufps::log::Level L>
    class AssimpLogStreamAdapter : public ::Assimp::LogStream
    {
    public:
        auto write(const char *msg) -> void override
        {
            ufps::log::log("{}", msg);
        }
    };

    auto to_native(const ::aiVector3D &v) -> ufps::Vector3
    {
        return {v.x, v.y, v.z};
    }

    auto channels_to_format(int num_channels) -> ufps::TextureFormat
    {
        switch (num_channels)
        {
            using enum ufps::TextureFormat;
        case 1:
            return R;
        case 3:
            return RGB;
        case 4:
            return RGBA;
        default:
            throw ufps::Exception("unsupported channel count: {}", num_channels);
        }
    }

    auto to_string(::aiTextureType type) -> std::string
    {
        const auto str = std::string{::aiTextureTypeToString(type)};
        return str;
    }

    auto get_texture_file_name(const ::aiMaterial *material, ::aiTextureType type) -> std::optional<std::filesystem::path>
    {
        if (material->GetTextureCount(type) == 0)
        {
            ufps::log::warn("no texture of type {} found", to_string(type));
            return std::nullopt;
        }

        auto path_str = ::aiString{};
        material->GetTexture(type, 0, &path_str);
        auto str = std::string{path_str.C_Str()};
        std::replace(str.begin(), str.end(), '\\', '/');
        const auto path = std::filesystem::path{str};
        const auto filename = path.filename();
        ufps::log::info("found texture: {} type: {}", filename.string(), to_string(type));

        return filename;
    }

}

namespace ufps
{
    auto load_texture(DataBufferView image_data) -> TextureData
    {
        auto width = int{};
        auto height = int{};
        auto num_channels = int{};

        ::stbi_set_flip_vertically_on_load(true);

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

        return {
            .width = static_cast<std::uint32_t>(width),
            .height = static_cast<std::uint32_t>(height),
            .format = channels_to_format(num_channels),
            .data = {{ptr, ptr + width * height * num_channels}},
        };
    }

    auto load_model(DataBufferView model_data, ResourceLoader &resource_loader, std::string format) -> std::vector<ModelData>
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

        auto importer = ::Assimp::Importer{};
        auto *scene = importer.ReadFileFromMemory(
            model_data.data(),
            model_data.size(),
            ::aiPostProcessSteps::aiProcess_Triangulate | ::aiPostProcessSteps::aiProcess_FlipUVs | ::aiPostProcessSteps::aiProcess_CalcTangentSpace,
            format.c_str());

        ensure(scene != nullptr, "failed to parse assimp scene");

        const auto loaded_meshes = std::span<::aiMesh *>(scene->mMeshes, scene->mMeshes + scene->mNumMeshes);
        const auto materials = std::span<::aiMaterial *>(scene->mMaterials, scene->mMaterials + scene->mNumMaterials);
        log::info("found {} meshes, {} materials {} lights", scene->mName.C_Str(), loaded_meshes.size(), materials.size(), scene->mNumLights);

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

            // for (auto i = 0; i < 27; ++i)
            // {
            //     auto type = static_cast<::aiTextureType>(i);
            //     const auto cnt = material->GetTextureCount(type);
            //     log::debug("{}: Texture type: {}, count: {}", i, ::aiTextureTypeToString(type), cnt);
            // }

            const auto albedo_filename =
                diffuse_color_count > 0 ? get_texture_file_name(material, ::aiTextureType_DIFFUSE) : get_texture_file_name(material, ::aiTextureType_BASE_COLOR);

            const auto normal_filename = get_texture_file_name(material, ::aiTextureType_NORMAL_CAMERA);
            const auto specular_filename = get_texture_file_name(material, ::aiTextureType_METALNESS);
            const auto roughness_filename = get_texture_file_name(material, ::aiTextureType_DIFFUSE_ROUGHNESS);
            const auto ao_filename = get_texture_file_name(material, ::aiTextureType_AMBIENT_OCCLUSION);

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
                .ambient_occlusion = std::nullopt};

            if (albedo_filename.has_value())
            {
                model.albedo = load_texture(resource_loader.load_data_buffer(std::format("textures/{}", albedo_filename->string())));
            }
            if (normal_filename.has_value())
            {
                model.normal = load_texture(resource_loader.load_data_buffer(std::format("textures/{}", normal_filename->string())));
            }
            if (specular_filename.has_value())
            {
                model.specular = load_texture(resource_loader.load_data_buffer(std::format("textures/{}", specular_filename->string())));
            }
            if (roughness_filename.has_value())
            {
                model.roughness = load_texture(resource_loader.load_data_buffer(std::format("textures/{}", normal_filename->string())));
            }
            if (ao_filename.has_value())
            {
                model.ambient_occlusion = load_texture(resource_loader.load_data_buffer(std::format("textures/{}", specular_filename->string())));
            }

            models.push_back(model);
        }

        return models;
    }
}