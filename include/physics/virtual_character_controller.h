#pragma once

#include <chrono>

#include "math/quaternion.h"
#include "math/vector3.h"
#include "physics/jolt.h"
#include "physics/physics_debug_renderer.h"

namespace ufps
{
    class VirtualCharacterController
    {
    public:
        VirtualCharacterController(::JPH::PhysicsSystem &ps);

        VirtualCharacterController(const VirtualCharacterController &) = delete;
        auto operator=(const VirtualCharacterController &) -> VirtualCharacterController & = delete;
        VirtualCharacterController(VirtualCharacterController &&) = delete;
        auto operator=(VirtualCharacterController &&) -> VirtualCharacterController & = delete;

        auto update(std::chrono::milliseconds delta) -> void;

        auto debug_draw(PhysicsDebugRenderer &renderer) -> void;

        auto position() const -> Vector3;
        auto rotation() const -> Quaternion;

    private:
        ::JPH::PhysicsSystem &_ps;
        ::JPH::RefConst<::JPH::Shape> _shape;
        ::JPH::Ref<::JPH::CharacterVirtual> _character;
    };
}