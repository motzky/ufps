#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "graphics/buffer.h"
#include "graphics/mesh_data.h"
#include "graphics/mesh_view.h"
#include "graphics/opengl.h"
#include "graphics/vertex_data.h"
#include "utils/string_unordered_map.h"

namespace ufps
{
    class MeshManager
    {
    public:
        MeshManager();
        auto load(std::string_view name, const MeshData &mesh_data) -> MeshView;

        auto mesh(std::string_view name) -> MeshView;

        auto mesh_names() const -> std::vector<std::string>;

        auto native_handle() const -> std::tuple<::GLuint, ::GLuint>;

        auto index_data(MeshView view) const -> std::span<const std::uint32_t>;
        auto vertex_data(MeshView view) const -> std::span<const VertexData>;

        auto to_string() const -> std::string;

    private:
        std::vector<VertexData> _vertex_data_cpu;
        std::vector<std::uint32_t> _index_data_cpu;
        Buffer _vertex_data_gpu;
        Buffer _index_data_gpu;
        StringUnorderedMap<MeshView> _mesh_lookup;
    };
}
