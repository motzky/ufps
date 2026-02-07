#pragma once

#include <cstdint>
#include <ranges>
#include <span>
#include <vector>

#include "graphics/buffer.h"
#include "graphics/utils.h"
#include "utils/ensure.h"

namespace ufps
{
    struct MaterialData
    {
        Color color;
        std::uint32_t albedo_texture_index;
        std::uint32_t normal_texture_index;
        std::uint32_t specular_texture_index;
        std::uint32_t roughness_texture_index;
        std::uint32_t ao_texture_index;
        std::uint32_t emissive_texture_index;
    };

    class MaterialManager
    {
    public:
        MaterialManager()
            : _material_data_cpu{},
              _material_data_gpu{sizeof(MaterialData), "material_manager_buffer"}
        {
        }

        template <class... Args>
        auto add(Args &&...args) -> std::uint32_t
        {
            const auto new_index = _material_data_cpu.size();
            _material_data_cpu.emplace_back(std::forward<Args>(args)...);
            resize_gpu_buffer(_material_data_cpu, _material_data_gpu);
            _material_data_gpu.write(std::as_bytes(std::span{_material_data_cpu.data(), _material_data_cpu.size()}), 0zu);

            return new_index;
        }

        auto data() const -> std::span<const MaterialData>
        {
            return _material_data_cpu;
        }

        auto native_handle() const
        {
            return _material_data_gpu.native_handle();
        }

    private:
        std::vector<MaterialData> _material_data_cpu;
        Buffer _material_data_gpu;
    };
}