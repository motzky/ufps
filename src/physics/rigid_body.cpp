#include "physics/rigid_body.h"

#include "math/vector3.h"
#include "physics/jolt.h"
#include "utils/exception.h"

namespace ufps
{
    RigidBody::RigidBody(::JPH::BodyID body_id, ::JPH::BodyInterface *body_interface)
        : _body_id{body_id},
          _body_interface{body_interface},
          _original_shape{body_interface->GetShape(_body_id)},
          _local_transform{{}, {1.f}, {}},
          _parent_transform{{}, {1.f}, {}},
          _applied_scale{1.f}
    {
    }

    auto RigidBody::native_handle() const -> ::JPH::BodyID
    {
        return _body_id;
    }

    auto RigidBody::position() const -> Vector3
    {
        return to_native(_body_interface->GetPosition(_body_id));
    }

    auto RigidBody::transform() const -> Transform
    {
        return {to_native(_body_interface->GetWorldTransform(_body_id)) * Matrix4{_applied_scale, Matrix4::Scale{}}};
    }

    auto RigidBody::local_transform() const -> Transform
    {
        return _local_transform;
    }

    auto RigidBody::parent_transform() const -> Transform
    {
        return _parent_transform;
    }

    auto RigidBody::set_local_transform(const Transform &transform) -> void
    {
        update_transforms(transform, _parent_transform);
    }

    auto RigidBody::set_parent_transform(const Transform &transform) -> void
    {
        update_transforms(_local_transform, transform);
    }

    auto RigidBody::update_transforms(const Transform &local, const Transform &parent) -> void
    {
        const auto world_transform = Transform{Matrix4{parent} * Matrix4{local}};

        if (world_transform.scale != _applied_scale)
        {
            const auto jolt_scale = to_jolt(world_transform.scale);

            auto scaled_result = _original_shape->ScaleShape(jolt_scale);

            if (scaled_result.HasError())
            {
                throw Exception("scale error: {}", scaled_result.GetError());
            }

            _body_interface->SetShape(_body_id, scaled_result.Get(), false, ::JPH::EActivation::Activate);

            _applied_scale = world_transform.scale;
        }

        _body_interface->SetPositionAndRotation(
            _body_id,
            to_jolt(world_transform.position),
            to_jolt(world_transform.rotation),
            ::JPH::EActivation::Activate);

        _local_transform = local;
        _parent_transform = parent;
    }

    auto RigidBody::description() const -> Description
    {
        return {
            .local_transform = _local_transform,
            .applied_scale = _applied_scale};
    }
}
