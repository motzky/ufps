#pragma once

#include "core/sparse_set.h"
#include "math/aabb.h"
#include "math/vector3.h"
#include "physics/jolt.h"
#include "physics/physics_layers.h"
#include "physics/rigid_body.h"
#include "physics/utils.h"

namespace ufps
{

    using RigidBodyHandle = SparseSet<RigidBody>::handle_type;

    class PhysicsSystem
    {
    public:
        PhysicsSystem();
        ~PhysicsSystem() = default;
        PhysicsSystem(const PhysicsSystem &) = delete;
        auto operator=(const PhysicsSystem &) -> PhysicsSystem & = delete;
        PhysicsSystem(PhysicsSystem &&) = delete;
        auto operator=(PhysicsSystem &&) -> PhysicsSystem & = delete;

        auto create_box(const AABB &aabb, const Vector3 &position, PhysicsLayer layer) -> RigidBodyHandle;

        constexpr auto rigid_body(this auto &&self, RigidBodyHandle handle);

        auto update() -> void;

    private:
        SimpleBroadPhaseLayer _broad_phase_layer;
        SimpleObjectVsBroadPhaseLayerFilter _object_vs_broadphase_layer_filter;
        SimpleObjectLayerPairFilter _object_layer_pair_filter;
        ::JPH::TempAllocatorImpl _temp_allocator;
        ::JPH::JobSystemThreadPool _job_system;
        ::JPH::PhysicsSystem _physics_system;
        SparseSet<RigidBody> _rigid_bodies;
    };

    constexpr auto PhysicsSystem::rigid_body(this auto &&self, RigidBodyHandle handle)
    {
        return self._rigid_bodies[handle];
    }
}