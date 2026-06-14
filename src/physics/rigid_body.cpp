#include "physics/rigid_body.h"

#include "math/vector3.h"
#include "physics/jolt.h"

namespace ufps
{
    RigidBody::RigidBody(::JPH::BodyID body_id, ::JPH::BodyInterface *body_interface)
        : _body_id{body_id},
          _body_interface{body_interface}
    {
    }

    auto RigidBody::position() const -> Vector3
    {
        return to_native(_body_interface->GetPosition(_body_id));
    }
}
