#pragma once

#include <cstdint>
#include <span>
#include <string>

#include "graphics/buffer.h"
#include "graphics/mesh_data.h"
#include "graphics/mesh_view.h"
#include "graphics/opengl.h"
#include "graphics/vertex_data.h"

namespace ufps
{
    class MeshManager
    {
    public:
        MeshManager();
        auto load(const MeshData &mesh_data) -> MeshView;

        auto native_handle() const -> std::tuple<::GLuint, ::GLuint>;

        auto index_data(MeshView view) -> std::span<std::uint32_t>;
        auto vertex_data(MeshView view) -> std::span<VertexData>;

        auto
        to_string() const -> std::string;

    private:
        std::vector<VertexData> _vertex_data_cpu;
        std::vector<std::uint32_t> _index_data_cpu;
        Buffer _vertex_data_gpu;
        Buffer _index_data_gpu;
    };
}
