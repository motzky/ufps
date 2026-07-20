#include "physics/virtual_character_controller.h"

#include "math/vector3.h"
#include "physics/jolt.h"
#include "physics/physics_layers.h"

namespace ufps
{
    VirtualCharacterController::VirtualCharacterController(::JPH::PhysicsSystem &ps)
        : _ps{ps},
          _shape{},
          _inner_shape{},
          _character{}
    {
        _shape = ::JPH::RotatedTranslatedShapeSettings(
                     ::JPH::Vec3(0.f, .5f * 1.35f + 0.3f, 0.f),
                     ::JPH::Quat::sIdentity(),
                     new ::JPH::CapsuleShape(.5f * 1.35f, .3f))
                     .Create()
                     .Get();

        _inner_shape = ::JPH::RotatedTranslatedShapeSettings(
                           ::JPH::Vec3(0.f, .5f * 1.35f + 0.3f, 0.f),
                           ::JPH::Quat::sIdentity(),
                           new ::JPH::CapsuleShape(.5f * 1.35f, .3f * .9f))
                           .Create()
                           .Get();

        auto settings = ::JPH::Ref{new ::JPH::CharacterVirtualSettings{}};
        settings->mShape = _shape;
        settings->mInnerBodyShape = _inner_shape;
        settings->mInnerBodyLayer = static_cast<::JPH::ObjectLayer>(PhysicsLayer::DYNAMIC);

        _character = new ::JPH::CharacterVirtual(settings, ::JPH::Vec3::sZero(), ::JPH::Quat::sIdentity(), 0, std::addressof(ps));

        _character->SetListener(this);
    }

    auto VirtualCharacterController::update(std::chrono::milliseconds delta) -> void
    {
        static auto temp_allocator = ::JPH::TempAllocatorImpl{10zu * 1024zu * 1024zu};

        const auto jolt_delta = 1.f / delta.count();

        auto new_velocity = Vector3{0.f, 0.f, -.5f};

        new_velocity += to_native(_ps.GetGravity()) * jolt_delta;

        _character->SetLinearVelocity(to_jolt(new_velocity));

        _character->Update(
            jolt_delta,
            -_character->GetUp() * _ps.GetGravity().Length(),
            _ps.GetDefaultBroadPhaseLayerFilter(static_cast<::JPH::ObjectLayer>(PhysicsLayer::DYNAMIC)),
            _ps.GetDefaultLayerFilter(static_cast<::JPH::ObjectLayer>(PhysicsLayer::DYNAMIC)),
            {},
            {},
            temp_allocator);
    }

    auto VirtualCharacterController::debug_draw(PhysicsDebugRenderer &renderer) -> void
    {
        _character->GetShape()->Draw(
            std::addressof(renderer),
            _character->GetCenterOfMassTransform(),
            ::JPH::Vec3::sOne(),
            ::JPH::Color::sOrange,
            false,
            true);
    }

    auto VirtualCharacterController::position() const -> Vector3
    {
        return to_native(_character->GetPosition());
    }

    auto VirtualCharacterController::rotation() const -> Quaternion
    {
        return to_native(_character->GetRotation());
    }

    auto VirtualCharacterController::OnContactAdded(
        [[maybe_unused]] const ::JPH::CharacterVirtual *inCharacter,
        [[maybe_unused]] const ::JPH::CharacterContact &inContact,
        // [[maybe_unused]] const ::JPH::BodyID &inBodyID2,
        // [[maybe_unused]] const ::JPH::SubShapeID &inSubShapeID2,
        // [[maybe_unused]] ::JPH::RVec3Arg inContactPosition,
        // [[maybe_unused]] ::JPH::Vec3Arg inContactNormal,
        [[maybe_unused]] ::JPH::CharacterContactSettings &ioSettings) -> void
    {
        // const auto position = inContact.mPosition;
        // const auto normal = -inContact.mContactNormal;

        log::debug("here");
    }
}