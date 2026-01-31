#include "resources/embedded_resource_loader.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "utils/data_buffer.h"
#include "utils/ensure.h"

namespace
{
    constexpr const std::uint8_t simple_vertex_shader[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/shaders/simple.vert"
#else
        0
#endif
    };

    constexpr const std::uint8_t simple_fragment_shader[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/shaders/simple.frag"
#else
        0
#endif
    };

    constexpr const std::uint8_t gbuffer_vertex_shader[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/shaders/gbuffer.vert"
#else
        0
#endif
    };

    constexpr const std::uint8_t gbuffer_fragment_shader[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/shaders/gbuffer.frag"
#else
        0
#endif
    };

    constexpr const std::uint8_t light_pass_vertex_shader[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/shaders/light_pass.vert"
#else
        0
#endif
    };

    constexpr const std::uint8_t light_pass_fragment_shader[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/shaders/light_pass.frag"
#else
        0
#endif
    };

    constexpr const std::uint8_t diamond_floor_albedo[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/textures/diamond_floor_albedo.png"
#else
        0
#endif
    };

    constexpr const std::uint8_t diamond_floor_normal[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/textures/diamond_floor_normal.png"
#else
        0
#endif
    };

    constexpr const std::uint8_t diamond_floor_specular[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/textures/diamond_floor_specular.png"
#else
        0
#endif
    };

    constexpr const std::uint8_t sm_corner_01_8_8_X_fbx[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/models/SM_Corner01_8_8_X.fbx"
#else
        0
#endif
    };

    constexpr const std::uint8_t metal_plate_02_base_colour[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/textures/MetalPlate02_BaseColor.png"
#else
        0
#endif
    };

    constexpr const std::uint8_t metal_plate_03_base_colour[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/textures/MetalPlate03_BaseColor.png"
#else
        0
#endif
    };

    constexpr const std::uint8_t MetalPlate01_BaseColor[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/textures/MetalPlate01_BaseColor.png"
#else
        0
#endif
    };

    constexpr const std::uint8_t Details03_BaseColor[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/textures/Details03_BaseColor.png"
#else
        0
#endif
    };

    constexpr const std::uint8_t DeatilsBG_BaseColor[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/textures/DeatilsBG_BaseColor.png"
#else
        0
#endif
    };

    constexpr const std::uint8_t T_Light_BC[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/textures/T_Light_BC.png"
#else
        0
#endif
    };

    constexpr const std::uint8_t Detail02_BaseColor[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/textures/Detail02_BaseColor.png"
#else
        0
#endif
    };

    constexpr const std::uint8_t Details01_BaseColor[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/textures/Details01_BaseColor.png"
#else
        0
#endif
    };

    constexpr const std::uint8_t Pipes01_BaseColor[] = {
#ifndef __INTELLISENSE__
#embed "../../assets/textures/Pipes01_BaseColor.png"
#else
        0
#endif
    };

    template <class T>
    auto to_container(std::span<const std::uint8_t> data) -> T
    {
        static_assert(sizeof(typename T::value_type) == 1);

        const auto *ptr = reinterpret_cast<const T::value_type *>(data.data());
        return T{ptr, ptr + data.size()};
    }
}

namespace ufps
{
    EmbeddedResourceLoader::EmbeddedResourceLoader()
    {
        _lookup = {
            {"models/SM_Corner01_8_8_X.fbx", sm_corner_01_8_8_X_fbx},

            {"shaders/simple.vert", simple_vertex_shader},
            {"shaders/simple.frag", simple_fragment_shader},
            {"shaders/gbuffer.vert", gbuffer_vertex_shader},
            {"shaders/gbuffer.frag", gbuffer_fragment_shader},
            {"shaders/light_pass.vert", light_pass_vertex_shader},
            {"shaders/light_pass.frag", light_pass_fragment_shader},

            {"textures/diamond_floor_albedo.png", diamond_floor_albedo},
            {"textures/diamond_floor_normal.png", diamond_floor_normal},
            {"textures/diamond_floor_specular.png", diamond_floor_specular},
            {"textures/MetalPlate02_BaseColor.png", metal_plate_02_base_colour},
            {"textures/MetalPlate03_BaseColor.png", metal_plate_03_base_colour},
            {"textures/MetalPlate01_BaseColor.png", MetalPlate01_BaseColor},
            {"textures/Details03_BaseColor.png", Details03_BaseColor},
            {"textures/DeatilsBG_BaseColor.png", DeatilsBG_BaseColor},
            {"textures/T_Light_BC.png", T_Light_BC},
            {"textures/Detail02_BaseColor.png", Detail02_BaseColor},
            {"textures/Details01_BaseColor.png", Details01_BaseColor},
            {"textures/Pipes01_BaseColor.png", Pipes01_BaseColor},
        };
    }

    auto EmbeddedResourceLoader::load_string(std::string_view name) -> std::string
    {
        const auto res = _lookup.find(name);
        expect(res != std::ranges::cend(_lookup), "resource {} does not exist", name);

        return to_container<std::string>(res->second);
    }

    auto EmbeddedResourceLoader::load_data_buffer(std::string_view name) -> DataBuffer
    {
        const auto res = _lookup.find(name);
        expect(res != std::ranges::cend(_lookup), "resource {} does not exist", name);

        return to_container<DataBuffer>(res->second);
    }

}