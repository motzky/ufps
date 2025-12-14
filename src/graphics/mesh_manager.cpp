#include "graphics/mesh_manager.h"

#include <cstdint>
#include <ranges>
#include <vector>

#include "graphics/buffer.h"
#include "graphics/mesh_data.h"
#include "graphics/utils.h"
#include "graphics/vertex_data.h"
#include "log.h"
#include "utils/formatter.h"

namespace ufps
{
    MeshManager::MeshManager()
        : _vertex_data_cpu{},
          _index_data_cpu{},
          _vertex_data_gpu{sizeof(VertexData), "vertex_mesh_data"},
          _index_data_gpu{sizeof(std::uint32_t), "index_mesh_data"}
    {
    }

    auto MeshManager::load(const MeshData &mesh_data) -> MeshView
    {
        const auto offset = _index_data_cpu.size();
        const auto base_vertex = _vertex_data_cpu.size();

        _vertex_data_cpu.append_range(mesh_data.vertices);
        _index_data_cpu.append_range(mesh_data.indices);

        resize_gpu_buffer(_vertex_data_cpu, _vertex_data_gpu, "vertex_mesh_data");

        auto vertex_data_view = DataBufferView{reinterpret_cast<const std::byte *>(_vertex_data_cpu.data()), _vertex_data_cpu.size() * sizeof(VertexData)};
        _vertex_data_gpu.write(vertex_data_view, 0zu);

        resize_gpu_buffer(_index_data_cpu, _index_data_gpu, "index_mesh_data");
        auto index_data_view = DataBufferView{reinterpret_cast<const std::byte *>(_index_data_cpu.data()), _index_data_cpu.size() * sizeof(std::uint32_t)};
        _index_data_gpu.write(index_data_view, 0zu);

        return {
            .vertex_offset = static_cast<std::uint32_t>(base_vertex),
            .vertex_count = static_cast<std::uint32_t>(mesh_data.vertices.size()),
            .index_offset = static_cast<std::uint32_t>(offset),
            .index_count = static_cast<std::uint32_t>(mesh_data.indices.size())};
    }

    auto MeshManager::native_handle() const -> std::tuple<::GLuint, ::GLuint>
    {
        return {_vertex_data_gpu.native_handle(), _index_data_gpu.native_handle()};
    }

    auto MeshManager::to_string() const -> std::string
    {
        return std::format("mesh manager: vertex count: {}, index count:", _vertex_data_cpu.size(), _index_data_cpu.size());
    }

}