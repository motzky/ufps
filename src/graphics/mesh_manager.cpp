#include "graphics/mesh_manager.h"

#include <cstdint>
#include <ranges>
#include <vector>

#include "graphics/buffer.h"
#include "graphics/vertex_data.h"
#include "log.h"
#include "utils/formatter.h"

namespace ufps
{
    MeshManager::MeshManager()
        : _mesh_data_cpu{},
          _mesh_data_gpu{sizeof(VertexData), "mesh_data"}
    {
        _mesh_data_cpu.reserve(1zu);
    }

    auto MeshManager::load(const std::vector<VertexData> &mesh) -> MeshView
    {
        const auto offset = _mesh_data_cpu.size();

        _mesh_data_cpu.append_range(mesh);

        const auto buffer_size_bytes = _mesh_data_cpu.size() * sizeof(VertexData);

        if (_mesh_data_gpu.size() <= buffer_size_bytes)
        {
            auto new_size = _mesh_data_gpu.size() * 2zu;
            while (new_size < buffer_size_bytes)
            {
                new_size *= 2zu;
            }

            log::info("growing mesh_data buffer {} -> {}", _mesh_data_gpu.size(), new_size);
            // opengl barrier in case gpu using previous frame
            ::glFinish();

            _mesh_data_gpu = Buffer{new_size, "mesh_data"};
        }

        auto mesh_view = DataBufferView{reinterpret_cast<const std::byte *>(_mesh_data_cpu.data()), buffer_size_bytes};
        _mesh_data_gpu.write(mesh_view, 0zu);

        return {.offset = static_cast<std::uint32_t>(offset), .count = static_cast<std::uint32_t>(mesh.size())};
    }

    auto MeshManager::native_handle() const -> ::GLuint
    {
        return _mesh_data_gpu.native_handle();
    }

    auto MeshManager::to_string() const -> std::string
    {
        return std::format("mesh manager: vertex count: {:x}", _mesh_data_cpu.size());
    }

}