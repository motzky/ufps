#include "physics/virtual_character_controller.h"

#include "physics/jolt.h"
#include "physics/physics_layers.h"

namespace ufps
{
    VirtualCharacterController::VirtualCharacterController(::JPH::PhysicsSystem &ps)
        : _ps{ps},
          _shape{},
          _character{}
    {
        _shape = ::JPH::RotatedTranslatedShapeSettings(
                     ::JPH::Vec3(0.f, .5f * 1.35f + 0.3f, 0.f),
                     ::JPH::Quat::sIdentity(),
                     new ::JPH::CapsuleShape(.5f * 1.35f, .3f))
                     .Create()
                     .Get();

        auto settings = ::JPH::Ref{new ::JPH::CharacterVirtualSettings{}};
        settings->mShape = _shape;

        _character = new ::JPH::CharacterVirtual(settings, ::JPH::Vec3::sZero(), ::JPH::Quat::sIdentity(), 0, std::addressof(ps));
    }

    auto VirtualCharacterController::update(std::chrono::milliseconds delta) -> void
    {
        static auto temp_allocator = ::JPH::TempAllocatorImpl{10zu * 1024zu * 1024zu};

        _character->Update(
            1.f / delta.count(),
            -_character->GetUp() * _ps.GetGravity().Length(),
            _ps.GetDefaultBroadPhaseLayerFilter(static_cast<::JPH::ObjectLayer>(PhysicsLayer::STATIC)),
            _ps.GetDefaultLayerFilter(static_cast<::JPH::ObjectLayer>(PhysicsLayer::STATIC)),
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

}