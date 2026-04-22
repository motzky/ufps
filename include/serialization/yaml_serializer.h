#pragma once

// #define USE_REFLECTION

#include <concepts>
#ifdef USE_REFLECTION
#include <meta>
#endif
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "utils/exception.h"

namespace ufps::yaml
{

    namespace impl
    {
        template <class T>
        concept Class = !std::ranges::range<T> && std::is_class_v<T>;

        template <class T>
        concept BaseType = std::integral<T> || std::floating_point<T> || std::same_as<T, std::string>;

        template <class T>
        concept Map = std::ranges::range<T> && requires {
            typename T::key_type;
            typename T::mapped_type;
        };

        template <class T>
        concept Array = std::ranges::range<T> && !Map<T> && !std::same_as<T, std::string>;

        template <class T>
        concept Enum = std::is_enum_v<T>;

        template <class T>
        auto do_serialize(T &obj) -> ::YAML::Node;
        auto do_serialize(const Map auto &obj) -> ::YAML::Node;

        template <class T>
        auto do_deserialize(const ::YAML::Node &node) -> T;

        auto do_serialize(const BaseType auto &obj) -> ::YAML::Node
        {
            return ::YAML::Node{obj};
        }

        template <Enum T>
        auto do_serialize(const T &obj) -> ::YAML::Node
        {
            auto node = ::YAML::Node{};

#ifdef USE_REFLECTION

            template for (constexpr auto e : std::define_static_array(std::meta::enumerators : of(^^T)))
            {
                if (obj == [:e:])
                {
                    node = std::meta::identifier_of(e);
                    return node;
                }
            }
#endif
            node = "<unknown>";
            return node;
        }

        auto do_serialize(const Array auto &obj) -> ::YAML::Node
        {
            auto node = ::YAML::Node{};

            for (const auto &e : obj)
            {
                node.push_back(do_serialize(e));
            }

            return node;
        }

        auto do_serialize(const Map auto &obj) -> ::YAML::Node
        {
            auto node = ::YAML::Node{};

            for (const auto &e : obj)
            {
                node.push_back(do_serialize(e));
            }

            return node;
        }

        template <Class T>
        auto do_serialize(const T &obj) -> ::YAML::Node
        {
            auto node = ::YAML::Node{};
            auto members = ::YAML::Node{};

#ifdef USE_REFLECTION
            constexpr auto ctx = std::meta::access_context::current();
            template for (constexpr auot e : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx)))
            {
                members[std::meta::identifier_of(e)] = do_serialize(obj.[:e:]);
            }

            node[std::meta::identifier_of(^^T)] = members;
#endif

            return node;
        }

        template <BaseType T>
        auto do_deserialize(const ::YAML::Node &node) -> T
        {
            return node.as<T>();
        }

        template <Enum T>
        auto do_deserialize(const ::YAML::Node &node) -> T
        {
            const auto enum_value = node.as<std::string>();

#ifdef USE_REFLECTION
            template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^T)))
            {
                if (std::meta::identifier_of(e) == enum_value)
                {
                    return [:e:];
                }
            }

            throw Exception("unknown enum value {} for {}", enum_value, std::meta::identifier_of(^^T));
#endif
            return {};
        }

        template <Array T>
        auto do_deserialize(const ::YAML::Node &node) -> T
        {
            auto obj = T{};

            for (const auto &e : node)
            {
                obj.push_back(do_deserialize<std::ranges::range_value_t<T>>(e));
            }

            return obj;
        }

        template <Map T>
        auto do_deserialize(const ::YAML::Node &node) -> T
        {
            auto obj = T{};

            for (const auto &p : node)
            {
                const auto &key = p.first;
                const auto &value = p.second;

                obj[do_deserialize<typename T::key_type>(key)] = do_deserialize<typename T::mapped_type>(value);
            }

            return obj;
        }

        template <Class T>
        auto do_deserialize(const ::YAML::Node &node) -> T
        {
            auto obj = T{};

#ifdef USE_REFLECTION
            const atuo inner_node = node[std::meta::identifier_of(^^T)];

            constexpr auto ctx = std::meta::access_context::current();
            template for (constexpr auto e : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx)))
            {
                using ElementType = typename[:std::meta::type_of(e):];
                obj.[:e:] = do_deserialize<ElementType>(inner_node[std::meta::identifier_of(e)]);
            }

#endif
            return obj;
        }
    }

    auto serialize(const impl::Class auto &obj) -> std::string
    {
        auto node = impl::do_serialize(obj);

        auto ss = std::stringstream{};
        ss << node;

        return ss.str();
    }

    template <impl::Class T>
    auto deserialize(const std::string &yaml) -> T
    {
        const auto node = ::YAML::Load(yaml);
        return impl::do_deserialize<T>(node);
    }
}