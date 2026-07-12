#pragma once

#include "math/transform.h"
#include "math/vector3.h"
#include "physics/jolt.h"

namespace ufps
{

    class RigidBody
    {
    public:
        struct Description
        {
            Matrix4 local_transform;
            Vector3 applied_scale;
        };

        RigidBody(::JPH::BodyID body_id, ::JPH::BodyInterface *body_interface);

        RigidBody(const RigidBody &) = delete;
        auto operator=(const RigidBody &) -> RigidBody & = delete;
        RigidBody(RigidBody &&) = default;
        auto operator=(RigidBody &&) -> RigidBody & = default;

        auto native_handle() const -> ::JPH::BodyID;

        auto position() const -> Vector3;
        auto transform() const -> Transform;
        auto local_transform() const -> Transform;
        auto parent_transform() const -> Transform;

        auto set_local_transform(const Transform &transform) -> void;
        auto set_parent_transform(const Transform &transform) -> void;

        auto description() const -> Description;

    private:
        auto update_transforms(const Transform &local, const Transform &parent) -> void;

        ::JPH::BodyID _body_id;
        ::JPH::BodyInterface *_body_interface;
        ::JPH::RefConst<::JPH::Shape> _original_shape;
        Transform _local_transform;
        Transform _parent_transform;
        Vector3 _applied_scale;
    };
}
