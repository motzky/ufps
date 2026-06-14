#include "physics/physics_system.h"

#include <cstdarg>
#include <cstdio>
#include <string_view>

#include "Jolt/Core/Memory.h"
#include "log.h"
#include "physics/jolt.h"
#include "physics/utils.h"
#include "utils/ensure.h"

namespace
{
    auto jolt_trace(const char *fmt, ...) -> void
    {
        auto list = ::va_list{};
        va_start(list, fmt);

        auto buffer = std::array<char, 1024zu>{};
        const auto write_count = ::vsnprintf(buffer.data(), sizeof(buffer), fmt, list);

        ::va_end(list);

        ufps::ensure(write_count > 0, "failed to jolt trace");

        ufps::log::info("jolt_trace: {}", std::string_view(buffer.data(), write_count));
    }

    auto jolt_init = []
    {
        ::JPH::RegisterDefaultAllocator();
        ::JPH::Trace = jolt_trace;
        ::JPH::Factory::sInstance = new ::JPH::Factory{};
        ::JPH::RegisterTypes();

        return true;
    }();
}

namespace ufps
{
    PhysicsSystem::PhysicsSystem()
        : _broad_phase_layer{},
          _object_vs_broadphase_layer_filter{},
          _object_layer_pair_filter{},
          _temp_allocator{10u * 1024u * 1024u},
          _job_system{::JPH::cMaxPhysicsJobs, ::JPH::cMaxPhysicsBarriers, static_cast<int>(std::thread::hardware_concurrency() - 1zu)},
          _physics_system{}
    {
        constexpr auto max_bodies = 1024u;
        constexpr auto max_body_mutexes = 0u;
        constexpr auto max_body_pairs = 1024u;
        constexpr auto max_contact_constraints = 1024u;

        _physics_system.Init(
            max_bodies,
            max_body_mutexes,
            max_body_pairs,
            max_contact_constraints,
            _broad_phase_layer,
            _object_vs_broadphase_layer_filter,
            _object_layer_pair_filter);

        _physics_system.SetGravity({0.f, -9.81f, 0.f});
    }

    auto PhysicsSystem::update() -> void
    {
        _physics_system.Update(1.f / 30.f, 1, &_temp_allocator, &_job_system);
    }

}