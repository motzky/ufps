#include "resources/file_resource_loader.h"

#include <cstddef>
#include <filesystem>

#include "resources/file.h"
#include "utils/auto_release.h"
#include "utils/data_buffer.h"
#include "utils/ensure.h"

namespace ufps
{

    FileResourceLoader::FileResourceLoader(const std::vector<std::filesystem::path> &roots)
        : _roots{roots}
    {
        for (const auto &root : _roots)
        {
            if (!std::filesystem::exists(root))
            {
                throw Exception("resource root does not exists: {}", root.string());
            }
        }
    }

    auto FileResourceLoader::load_string(std::string_view name) -> std::string
    {
        for (const auto &root : _roots)
        {
            auto path = root / name;

            if (std::filesystem::exists(path))
            {
                const auto file = File{path};
                auto str_view = file.as_string();

                return {str_view.data(), str_view.length()};
            }
        }

        throw Exception("cannot find {}", name);
    }

    auto FileResourceLoader::load_data_buffer(std::string_view name) -> DataBuffer
    {
        for (const auto &root : _roots)
        {
            auto path = root / name;

            if (std::filesystem::exists(path))
            {
                auto file = File{path};

                auto span = file.as_bytes();

                return {span.data(), span.data() + span.size_bytes()};
            }
        }

        throw Exception("cannot find {}", name);
    }

    auto FileResourceLoader::resources(std::string_view type) -> std::vector<std::string>
    {
        return _roots |
               std::views::transform(
                   [&](const auto &e)
                   {
                       const auto dir_iter = std::filesystem::directory_iterator{e / type};
                       return dir_iter |
                              std::views::filter([&](const auto &e)
                                                 { return e.is_regular_file(); }) |
                              std::views::transform([&](const auto &e)
                                                    { return std::format("{}/{}", type, e.path().filename().string()); }) |
                              std::ranges::to<std::vector>();
                   }) |
               std::views::join | std::ranges::to<std::vector>();
    }
}