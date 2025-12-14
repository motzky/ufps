#pragma once

#include "graphics/material_manager.h"
#include "graphics/mesh_view.h"
#include "math/transform.h"

namespace ufps
{
    struct Entity
    {
        MeshView mesh_view;
        Transform transform;
        MaterialKey material_key;
    };
}