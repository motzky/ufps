#include "physics/physics_system.h"

#include <contracts>
#include <cstdarg>
#include <cstdio>
#include <optional>
#include <string_view>

#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"

#include "log.h"
#include "math/vector3.h"
#include "physics/jolt.h"
#include "physics/physics_debug_renderer.h"
#include "physics/physics_layers.h"
#include "physics/rigid_body.h"
#include "physics/utils.h"
#include "utils/ensure.h"
#include "utils/formatter.h"

namespace
{
    auto to_activation(ufps::PhysicsLayer layer) -> ::JPH::EActivation
    {
        switch (layer)
        {
            using enum ufps::PhysicsLayer;
        case STATIC:
            return ::JPH::EActivation::DontActivate;
        case DYNAMIC:
            return ::JPH::EActivation::Activate;
        }

        throw ufps::Exception("unknown layer type: {}", layer);
    }

    auto to_motion(ufps::PhysicsLayer layer) -> ::JPH::EMotionType
    {
        switch (layer)
        {
            using enum ufps::PhysicsLayer;
        case STATIC:
            return ::JPH::EMotionType::Static;
        case DYNAMIC:
            return ::JPH::EMotionType::Dynamic;
        }

        throw ufps::Exception("unknown layer type: {}", layer);
    }

    auto jolt_trace(const char *fmt, ...) -> void
    {
        ::va_list list;
        va_start(list);

        auto buffer = std::array<char, 1024zu>{};
        const auto write_count = ::vsnprintf(buffer.data(), sizeof(buffer), fmt, list);

        ::va_end(list);

        ufps::ensure(write_count > 0, "failed to jolt trace");

        const auto error_str = std::string_view(buffer.data(), write_count);
        if (error_str.starts_with("Error"))
        {
            throw ufps::Exception("{}", error_str);
        }

        ufps::log::info("jolt_trace: {}", error_str);
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
    PhysicsSystem::PhysicsSystem(DebugRenderMode debug_render_mode)
        : _broad_phase_layer{},
          _object_vs_broadphase_layer_filter{},
          _object_layer_pair_filter{},
          _temp_allocator{10u * 1024u * 1024u},
          _job_system{::JPH::cMaxPhysicsJobs, ::JPH::cMaxPhysicsBarriers, static_cast<int>(std::thread::hardware_concurrency() - 1zu)},
          _physics_system{},
          _rigid_bodies{},
          _debug_renderer{debug_render_mode == DebugRenderMode::ON ? std::make_optional<PhysicsDebugRenderer>() : std::nullopt}

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

    auto PhysicsSystem::create_box(const AABB &aabb, const Vector3 &position, PhysicsLayer layer) -> RigidBodyHandle
    {
        const auto half_extents =
            Vector3{(aabb.max.x - aabb.min.x) / 2.f, (aabb.max.y - aabb.min.y) / 2.f, (aabb.max.z - aabb.min.z) / 2.f};

        auto box_shape_settings = ::JPH::BoxShapeSettings{to_jolt(half_extents)};
        box_shape_settings.SetEmbedded();

        auto box_result = box_shape_settings.Create();
        if (box_result.HasError())
        {
            throw Exception("box error: {}", box_result.GetError());
        }

        const auto &box = box_result.Get();

        const auto body_settings = ::JPH::BodyCreationSettings{
            box,
            to_jolt(position),
            ::JPH::Quat::sIdentity(),
            to_motion(layer),
            static_cast<::JPH::ObjectLayer>(layer)};

        auto &interface = _physics_system.GetBodyInterface();

        const auto body_id = interface.CreateAndAddBody(body_settings, to_activation(layer));

        return _rigid_bodies.emplace(body_id, std::addressof(interface));
    }

    auto PhysicsSystem::create_rigid_body(const RigidBody::Description &description) -> RigidBodyHandle
    {
        const auto transform = Transform{description.local_transform};
        const auto handle = create_box({{-1.f}, {1.f}}, transform.position, ufps::PhysicsLayer::STATIC);
        _rigid_bodies[handle]->set_local_transform(transform);

        return handle;
    }

    auto PhysicsSystem::duplicate_rigid_body(RigidBodyHandle handle) -> RigidBodyHandle
    {
        const auto &rb = rigid_body(handle);
        contract_assert(rb);

        return create_rigid_body(rb->description());
    }

    auto PhysicsSystem::remove_rigid_body(RigidBodyHandle handle) -> void
    {
        const auto &rb = rigid_body(handle);
        contract_assert(rb);

        _physics_system.GetBodyInterface().RemoveBody(rb->native_handle());

        _rigid_bodies.remove(handle);
    }

    auto PhysicsSystem::update() -> void
    {
        _physics_system.Update(1.f / 30.f, 1, &_temp_allocator, &_job_system);

        if (_debug_renderer)
        {
            static const auto settings = ::JPH::BodyManager::DrawSettings{};
            _physics_system.DrawBodies(settings, &*_debug_renderer);
        }
    }

    auto PhysicsSystem::debug_renderer() -> std::optional<PhysicsDebugRenderer &>
    {
        return _debug_renderer.transform([](auto &e) -> decltype(auto)
                                         { return e; });
    }
}