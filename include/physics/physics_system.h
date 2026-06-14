#pragma once

#include "physics/jolt.h"
#include "physics/utils.h"

namespace ufps
{

    class PhysicsSystem
    {
    public:
        PhysicsSystem();
        ~PhysicsSystem() = default;
        PhysicsSystem(const PhysicsSystem &) = delete;
        auto operator=(const PhysicsSystem &) -> PhysicsSystem & = delete;
        PhysicsSystem(PhysicsSystem &&) = delete;
        auto operator=(PhysicsSystem &&) -> PhysicsSystem & = delete;

        auto update() -> void;

    private:
        SimpleBroadPhaseLayer _broad_phase_layer;
        SimpleObjectVsBroadPhaseLayerFilter _object_vs_broadphase_layer_filter;
        SimpleObjectLayerPairFilter _object_layer_pair_filter;
        ::JPH::TempAllocatorImpl _temp_allocator;
        ::JPH::JobSystemThreadPool _job_system;
        ::JPH::PhysicsSystem _physics_system;
    };

}