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

    auto RigidBody::position() const -> Vector3
    {
        return to_native(_body_interface->GetPosition(_body_id));
    }

    auto RigidBody::set_parent_transform(const Transform &transform) -> void
    {
        const auto world_transform = Transform{Matrix4{transform} * Matrix4{_local_transform}};

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
    }
}
