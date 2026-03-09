#pragma once

#include <vector>

#include "core/sub_mesh.h"
#include "math/aabb.h"
#include "math/transform.h"

namespace ufps
{
    struct Entity
    {
        std::string name;
        std::vector<SubMesh> sub_meshes;
        Transform transform;
        AABB aabb;
    };
}