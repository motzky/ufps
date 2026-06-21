#pragma once

#if !defined(JPH_DEBUG_RENDERER)
#define JPH_DEBUG_RENDERER
#endif

#include <Jolt/Jolt.h>

#include <Jolt/Core/Core.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterMask.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterMask.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/EActivation.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Renderer/DebugRenderer.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

#include "graphics/color.h"
#include "math/quaternion.h"
#include "math/vector3.h"

namespace ufps
{
    inline auto to_native(const ::JPH::Vec3 &vec) -> Vector3
    {
        return {vec.GetX(), vec.GetY(), vec.GetZ()};
    }

    inline auto to_jolt(const Vector3 vec) -> ::JPH::Vec3
    {
        return {vec.x, vec.y, vec.z};
    }

    inline auto to_native(const ::JPH::Color &c) -> Color
    {
        return {c.r / 255.f, c.g / 255.f, c.b / 255.f};
    }

    inline auto to_jolt(const Color color) -> ::JPH::Vec3
    {
        return {color.r, color.g, color.b};
    }

    inline auto to_jolt(const Quaternion q) -> ::JPH::Quat
    {
        return {q.x, q.y, q.z, q.w};
    }
}