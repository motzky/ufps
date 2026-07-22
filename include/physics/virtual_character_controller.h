#pragma once

#include <chrono>

#include "math/quaternion.h"
#include "math/vector3.h"
#include "physics/jolt.h"
#include "physics/physics_debug_renderer.h"

namespace ufps
{
    class VirtualCharacterController : public ::JPH::CharacterContactListener
    {
    public:
        VirtualCharacterController(::JPH::PhysicsSystem &ps);

        VirtualCharacterController(const VirtualCharacterController &) = delete;
        auto operator=(const VirtualCharacterController &) -> VirtualCharacterController & = delete;
        VirtualCharacterController(VirtualCharacterController &&) = delete;
        auto operator=(VirtualCharacterController &&) -> VirtualCharacterController & = delete;

        auto update(std::chrono::milliseconds delta) -> void;

        auto set_walk_direction(const Vector3 &walk_direction) -> void;

        auto debug_draw(PhysicsDebugRenderer &renderer) -> void;

        auto position() const -> Vector3;
        auto rotation() const -> Quaternion;

    protected:
        auto OnContactAdded(
            const ::JPH::CharacterVirtual *inCharacter,
            const ::JPH::CharacterContact &inContact,
            ::JPH::CharacterContactSettings &ioSettings) -> void override;

    private:
        ::JPH::PhysicsSystem &_ps;
        ::JPH::RefConst<::JPH::Shape> _shape;
        ::JPH::RefConst<::JPH::Shape> _inner_shape;
        ::JPH::Ref<::JPH::CharacterVirtual> _character;
        Vector3 _walk_direction;
    };
}