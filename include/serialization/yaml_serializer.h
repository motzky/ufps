#pragma once

#include <concepts>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace ufps::yaml
{

    namespace impl
    {
        template <class T>
        auto serialize(T &obj) -> ::YAML::Node
        {
            auto node = ::YAML::Node{};

            return node;
        }

        template <class T>
        struct SerializeRange;

        template <class T>
        struct SerializeRange<std::vector<T>>
        {
            auto operator()(::YAML::Node &node, const std::vector<T> &vec) -> void
            {
                for (const auto &v : vec)
                {
                    if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T> || std::same_as<T, std::string>)
                    {
                        node.push_back(v);
                    }
                    else
                    {
                        node.push_back(serialize(v));
                    }
                }
            }
        };

        template <class K, class V>
        struct SerializeRange<std::unordered_map<K, V>>
        {
            auto operator()(::YAML::Node &node, const std::unordered_map<K, V> &map) -> void
            {
                for (const auto &[k, v] : map)
                {
                    node[k] = v;
                }
            }
        };

        template <>
        struct SerializeRange<std::string>
        {
            auto operator()(::YAML::Node &node, const std::string &str) -> void
            {
                node = str;
            }
        };
    }

    template <class T>
    auto serialize(T &&obj) -> std::string
    {
        auto node = ::YAML::Node{};
        auto members = ::YAML::Node{};

        auto ss = std::stringstream{};
        ss << node;

        return ss.str();
    }
}