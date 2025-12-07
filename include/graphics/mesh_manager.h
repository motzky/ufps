#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "graphics/buffer.h"
#include "graphics/mesh_view.h"
#include "graphics/opengl.h"
#include "graphics/vertex_data.h"

namespace ufps
{
    class MeshManager
    {
    public:
        MeshManager();
        auto load(const std::vector<VertexData> &mesh) -> MeshView;

        auto native_handle() const -> ::GLuint;

        auto to_string() const -> std::string;

    private:
        std::vector<VertexData> _mesh_data_cpu;
        Buffer _mesh_data_gpu;
    };
}
