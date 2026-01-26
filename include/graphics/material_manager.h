#pragma once

#include <compare>
#include <cstdint>
#include <flat_map>
#include <ranges>
#include <string>
#include <vector>

#include "graphics/color.h"
#include "graphics/multi_buffer.h"
#include "graphics/persistent_buffer.h"
#include "graphics/utils.h"
#include "utils/ensure.h"
#include "utils/formatter.h"

namespace ufps
{

    class MaterialKey
    {
    public:
        constexpr MaterialKey(std::uint32_t key) : _key{key}
        {
        }

        constexpr auto get() const -> std::uint32_t
        {
            return _key;
        }

        constexpr auto operator*() const -> std::uint32_t
        {
            return get();
        }

        constexpr auto operator<=>(const MaterialKey &) const = default;

        constexpr auto to_string() const -> std::string
        {
            return std::to_string(_key);
        }

    private:
        std::uint32_t _key;
    };

    struct MaterialData
    {
        Color color;
        std::uint32_t albedo_texture_index;
        std::uint32_t normal_texture_index;
        std::uint32_t specular_texture_index;
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
        auto add(Args &&...args) -> MaterialKey
        {
            static auto key_num = 0u;
            const auto key = MaterialKey{key_num++};

            _material_data_cpu.emplace(key, MaterialData{std::forward<Args>(args)...});
            resize_gpu_buffer(data(), _material_data_gpu);

            return key;
        }

        auto operator[](MaterialKey key) -> MaterialData &
        {
            const auto element = _material_data_cpu.find(key);
            expect(element != std::ranges::cend(_material_data_cpu), "key {} does not exit", key);

            return element->second;
        }

        auto remove(MaterialKey key) -> void
        {
            _material_data_cpu.erase(key);
        }

        auto index(MaterialKey key) -> std::uint32_t
        {
            const auto element = _material_data_cpu.find(key);
            expect(element != std::ranges::cend(_material_data_cpu), "could not find key: {}", key);

            return static_cast<std::uint32_t>(std::ranges::distance(std::ranges::cbegin(_material_data_cpu), element));
        }

        auto data() const -> const std::vector<MaterialData> &
        {
            return _material_data_cpu.values();
        }

        auto sync() -> void
        {
            const auto &values = _material_data_cpu.values();
            _material_data_gpu.write(std::as_bytes(std::span{values.data(), values.size()}), 0zu);
        }

        auto native_handle() const
        {
            return _material_data_gpu.native_handle();
        }

        auto advance() -> void
        {
            _material_data_gpu.advance();
        }

    private:
        std::flat_map<MaterialKey, MaterialData> _material_data_cpu;
        MultiBuffer<PersistentBuffer> _material_data_gpu;
    };
}