#include "resources/file_resource_loader.h"

#include <cstddef>
#include <filesystem>

#include "resources/file.h"
#include "utils/auto_release.h"
#include "utils/data_buffer.h"
#include "utils/ensure.h"

namespace ufps
{

    FileResourceLoader::FileResourceLoader(const std::filesystem::path &root)
        : _root{root}
    {
    }

    auto FileResourceLoader::load_string(std::string_view name) -> std::string
    {
        auto file = File{_root / name};

        auto str_view = file.as_string();

        return {str_view.data(), str_view.length()};
    }

    auto FileResourceLoader::load_data_buffer(std::string_view name) -> DataBuffer
    {
        auto file = File{_root / name};

        auto span = file.as_bytes();

        return {span.data(), span.data() + span.size_bytes()};
    }

    auto FileResourceLoader::resources(std::string_view type) -> std::vector<std::string>
    {
        const auto dir_iter = std::filesystem::directory_iterator{_root / type};

        return dir_iter |
               std::views::filter([&](const auto &e)
                                  { return e.is_regular_file(); }) |
               std::views::transform([&](const auto &e)
                                     { return std::format("{}\\{}", type, e.path().filename().string()); }) |
               std::ranges::to<std::vector>();
    }
}