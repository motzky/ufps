#include "graphics/utils.h"

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
#include "utils/data_buffer.h"
#include "utils/ensure.h"

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

    auto load_model(DataBufferView model_data, std::string format) -> std::vector<ModelData>
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
        log::info("found {} meshes", loaded_meshes.size());

        auto models = std::vector<ModelData>{};

        for (const auto *mesh : loaded_meshes)
        {
            log::info("found mesh: {}", mesh->mName.C_Str());

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

            models.push_back(
                {.mesh_data = {.vertices = vertices(positions, normals, tangents, bitangents, uvs),
                               .indices = std::move(indices)},
                 .albedo = std::nullopt,
                 .normal = std::nullopt,
                 .specular = std::nullopt});
        }

        return models;
    }
}