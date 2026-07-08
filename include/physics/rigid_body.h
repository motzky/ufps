#pragma once

#include "math/transform.h"
#include "math/vector3.h"
#include "physics/jolt.h"

namespace ufps
{

    class RigidBody
    {
    public:
        RigidBody(::JPH::BodyID body_id, ::JPH::BodyInterface *body_interface);

        RigidBody(const RigidBody &) = delete;
        auto operator=(const RigidBody &) -> RigidBody & = delete;
        RigidBody(RigidBody &&) = default;
        auto operator=(RigidBody &&) -> RigidBody & = default;

        auto position() const -> Vector3;

        auto set_parent_transform(const Transform &transform) -> void;

    private:
        ::JPH::BodyID _body_id;
        ::JPH::BodyInterface *_body_interface;
        ::JPH::RefConst<::JPH::Shape> _original_shape;
        Transform _local_transform;
        Vector3 _applied_scale;
    };
}
