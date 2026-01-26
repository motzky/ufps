#pragma once

#include <cstdint>
#include <string>

#include "graphics/mesh_view.h"
#include "math/transform.h"

namespace ufps
{
    struct Entity
    {
        std::string name;
        MeshView mesh_view;
        Transform transform;
        std::uint32_t material_index;
    };
}