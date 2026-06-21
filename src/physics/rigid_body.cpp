#include "physics/rigid_body.h"

#include "math/vector3.h"
#include "physics/jolt.h"
#include "utils/exception.h"

namespace ufps
{
    RigidBody::RigidBody(::JPH::BodyID body_id, ::JPH::BodyInterface *body_interface)
        : _body_id{body_id},
          _body_interface{body_interface},
          _local_transform{{}, {1.f}, {}},
          _parent_transform{{}, {1.f}, {}}
    {
    }

    auto RigidBody::position() const -> Vector3
    {
        return to_native(_body_interface->GetPosition(_body_id));
    }

    auto RigidBody::set_parent_transform(const Transform &transform) -> void
    {
        const auto scale_amount = Vector3{1.f} + transform.scale - _parent_transform.scale;

        _parent_transform = transform;

        const auto new_transform = Transform{Matrix4{_parent_transform} * Matrix4{_local_transform}};

        _body_interface->SetPositionAndRotation(
            _body_id,
            to_jolt(new_transform.position),
            to_jolt(new_transform.rotation),
            ::JPH::EActivation::Activate);

        const auto jolt_scale = to_jolt(scale_amount);

        if (!jolt_scale.IsNearZero())
        {
            const auto shape = _body_interface->GetShape(_body_id);
            auto scaled_result = shape->ScaleShape(jolt_scale);

            if (scaled_result.HasError())
            {
                throw Exception("scale error: {}", scaled_result.GetError());
            }

            _body_interface->SetShape(_body_id, scaled_result.Get(), false, ::JPH::EActivation::Activate);
        }
    }
}
