#pragma once

#include <meta>
#include <ranges>
#include <string>
#include <unordered_map>

// #include <Jolt/Jolt.h>

// #include <Jolt/Math/Vec3.h>

// #include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

#include "physics/physics_layers.h"
#include "utils/ensure.h"

namespace ufps
{

    // class SimpleBroadPhaseLayer : public ::JPH::BroadPhaseLayerInterface
    // {
    // public:
    //     virtual auto GetNumBroadPhaseLayers() const -> ::JPH::uint override
    //     {
    //         return std::ranges::size(std::meta::enumerators_of(^^PhysicsLayer));
    //     }

    //     virtual auto GetBroadPhaseLayer(::JPH::ObjectLayer layer) const -> ::JPH::BroadPhaseLayer override
    //     {
    //         return ::JPH::BroadPhaseLayer{static_cast<::JPH::BroadPhaseLayer::Type>(layer)};
    //     }

    //     virtual auto GetBroadPhaseLayerName(::JPH::BroadPhaseLayer layer) const -> const char * override
    //     {
    //         const auto native_layer = PhysicsLayer{layer.GetValue()};

    //         static auto lookup = []
    //         {
    //             auto result = std::unordered_map<PhysicsLayer, std::string>{};
    //             template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^PhysicsLayer)))
    //             {
    //                 result[[:e:]] = std::string(std::meta::identifier_of(e));
    //             }
    //             return result;
    //         }();

    //         const auto find = lookup.find(native_layer);

    //         // ensure(find != std::ranges::cend(lookup), "could not find layer: {}", native_layer);
    //         return find->second.c_str();

    // private:
    // };
}