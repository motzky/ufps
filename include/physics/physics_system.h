#pragma once

#include <optional>

#include "core/sparse_set.h"
#include "math/aabb.h"
#include "math/vector3.h"
#include "physics/jolt.h"
#include "physics/physics_debug_renderer.h"
#include "physics/physics_layers.h"
#include "physics/rigid_body.h"
#include "physics/utils.h"
#include "physics/virtual_character_controller.h"

namespace ufps
{
    using RigidBodyHandle = SparseSet<RigidBody>::handle_type;

    enum class DebugRenderMode
    {
        ON,
        OFF
    };

    class PhysicsSystem
    {
    public:
        PhysicsSystem(DebugRenderMode debug_render = DebugRenderMode::OFF);
        ~PhysicsSystem() = default;
        PhysicsSystem(const PhysicsSystem &) = delete;
        auto operator=(const PhysicsSystem &) -> PhysicsSystem & = delete;
        PhysicsSystem(PhysicsSystem &&) = delete;
        auto operator=(PhysicsSystem &&) -> PhysicsSystem & = delete;

        auto create_box(const AABB &aabb, const Vector3 &position, PhysicsLayer layer) -> RigidBodyHandle;

        auto create_rigid_body(const RigidBody::Description &description) -> RigidBodyHandle;
        auto duplicate_rigid_body(RigidBodyHandle handle) -> RigidBodyHandle;
        auto remove_rigid_body(RigidBodyHandle handle) -> void;

        constexpr auto rigid_body(this auto &&self, RigidBodyHandle handle);

        auto update() -> void;

        auto debug_renderer() -> std::optional<PhysicsDebugRenderer &>;

    private:
        SimpleBroadPhaseLayer _broad_phase_layer;
        SimpleObjectVsBroadPhaseLayerFilter _object_vs_broadphase_layer_filter;
        SimpleObjectLayerPairFilter _object_layer_pair_filter;
        ::JPH::TempAllocatorImpl _temp_allocator;
        ::JPH::JobSystemThreadPool _job_system;
        ::JPH::PhysicsSystem _physics_system;
        SparseSet<RigidBody> _rigid_bodies;
        std::optional<PhysicsDebugRenderer> _debug_renderer;
        std::unique_ptr<VirtualCharacterController> _player_controller;
    };

    constexpr auto PhysicsSystem::rigid_body(this auto &&self, RigidBodyHandle handle)
    {
        return self._rigid_bodies[handle];
    }
}