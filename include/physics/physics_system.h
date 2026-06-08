#pragma once

#include "physics/utils.h"

namespace ufps
{

    class PhysicsSystem
    {
    public:
        PhysicsSystem();
        ~PhysicsSystem();
        PhysicsSystem(const PhysicsSystem &) = delete;
        auto operator=(const PhysicsSystem &) -> PhysicsSystem & = delete;
        PhysicsSystem(PhysicsSystem &&) = delete;
        auto operator=(PhysicsSystem &&) -> PhysicsSystem & = delete;

        auto update() -> void;

    private:
        // SimpleBroadPhaseLayer _broad_phase_layers;
    };

}