#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "serialization/yaml_serializer.h"

struct Simple
{
    int a;

    auto operator==(const Simple &) const -> bool = default;
};

struct MultiMember
{
    int a;
    float b;
    std::string c;
    bool d;

    auto operator==(const MultiMember &) const -> bool = default;
};

struct Array
{
    std::vector<int> a;

    auto operator==(const Array &) const -> bool = default;
};

struct ArrayOfStruct
{
    std::vector<Simple> v;

    auto operator==(const ArrayOfStruct &) const -> bool = default;
};

struct Map
{
    std::unordered_map<std::string, int> a;

    auto operator==(const Map &) const -> bool = default;
};

struct NestedStruct
{
    MultiMember m;

    auto operator==(const NestedStruct &) const -> bool = default;
};

enum class Fruit
{
    APPLE,
    BANANA
};

struct FruitStruct
{
    Fruit f;

    auto operator==(const FruitStruct &) const -> bool = default;
};

TEST(yaml_serialization, simple_struct)
{
    const auto result = ufps::yaml::serialize(Simple{.a = 12});

    const auto expected = R"(Simple:
  a: 12)";

    ASSERT_EQ(result, expected);
}

TEST(yaml_serialization, multi_member_struct)
{
    const auto result = ufps::yaml::serialize(
        MultiMember{
            .a = 12,
            .b = 3.1,
            .c = "hello world",
            .d = true,
        });

    const auto expected =
        R"(MultiMember:
  a: 12
  b: 3.1
  c: hello world
  d: true)";

    ASSERT_EQ(result, expected);
}

TEST(yaml_serialization, array_struct)
{
    const auto result = ufps::yaml::serialize(Array{.a = {1, 2, 3, 4, 5, 7}});
    const auto expected =
        R"(Array:
  a:
    - 1
    - 2
    - 3
    - 4
    - 5
    - 7)";

    ASSERT_EQ(result, expected);
}

TEST(yaml_serialization, array_of_member_struct)
{
    const auto result = ufps::yaml::serialize(
        ArrayOfStruct{
            .v =
                {
                    {.a = 10},
                    {.a = 20},
                    {.a = 30},
                },
        });
    const auto expected =
        R"(ArrayOfStruct:
  v:
    - Simple:
        a: 10
    - Simple:
        a: 20
    - Simple:
        a: 30)";

    ASSERT_EQ(result, expected);
}

TEST(yaml_serialization, map_struct)
{
    const auto result = ufps::yaml::serialize(
        Map{
            .a =
                {
                    {"1", 1},
                    {"2", 2},
                    {"3", 3},
                },
        });
    const auto expected =
        R"(Map:
  a:
    2: 2
    3: 3
    1: 1)";

    ASSERT_EQ(result, expected);
}

TEST(yaml_serialization, nested_struct)
{
    const auto result = ufps::yaml::serialize(
        NestedStruct{
            .m = {
                .a = 12,
                .b = 3.1,
                .c = "hello world",
                .d = true,
            }});
    const auto expected =
        R"(NestedStruct:
  m:
    MultiMember:
      a: 12
      b: 3.1
      c: hello world
      d: true)";

    ASSERT_EQ(result, expected);
}

TEST(yaml_serialization, enum_struct)
{
    const auto result = ufps::yaml::serialize(FruitStruct{.f = Fruit::APPLE});
    const auto expected =
        R"(FruitStruct:
  f: APPLE)";

    ASSERT_EQ(result, expected);
}

TEST(yaml_deserialization, simple_struct)
{
    const auto yaml =
        R"(Simple:
  a: 12)";

    const auto result = ufps::yaml::deserialize<Simple>(yaml);
    const auto expected = Simple{.a = 12};

    ASSERT_EQ(result, expected);
}

TEST(yaml_deserialization, multi_member_struct)
{
    const auto yaml =
        R"(MultiMember:
  a: 12
  b: 3.1
  c: hello world
  d: true)";
    const auto result = ufps::yaml::deserialize<MultiMember>(yaml);
    const auto expected = MultiMember{
        .a = 12,
        .b = 3.1,
        .c = "hello world",
        .d = true,
    };

    ASSERT_EQ(result, expected);
}

TEST(yaml_deserialization, array_struct)
{
    const auto yaml =
        R"(Array:
   a:
     - 1
     - 2
     - 3
     - 4
     - 5
     - 7)";
    const auto result = ufps::yaml::deserialize<Array>(yaml);
    const auto expected = Array{.a = {1, 2, 3, 4, 5, 7}};

    ASSERT_EQ(result, expected);
}

TEST(yaml_deserialization, array_of_member_struct)
{
    const auto yaml =
        R"(ArrayOfStruct:
   v:
     - Simple:
         a: 10
     - Simple:
         a: 20
     - Simple:
         a: 30)";
    const auto result = ufps::yaml::deserialize<ArrayOfStruct>(yaml);
    const auto expected = ArrayOfStruct{
        .v =
            {
                {.a = 10},
                {.a = 20},
                {.a = 30},
            },
    };

    ASSERT_EQ(result, expected);
}

TEST(yaml_deserialization, map_struct)
{
    const auto yaml =
        R"(Map:
   a:
     2: 2
     3: 3
     1: 1)";
    const auto result = ufps::yaml::deserialize<Map>(yaml);
    const auto expected = Map{
        .a =
            {
                {"1", 1},
                {"2", 2},
                {"3", 3},
            },
    };

    ASSERT_EQ(result, expected);
}

TEST(yaml_deserialization, nested_struct)
{
    const auto yaml =
        R"(NestedStruct:
   m:
     MultiMember:
       a: 12
       b: 3.1
       c: hello world
       d: true)";
    const auto result = ufps::yaml::deserialize<NestedStruct>(yaml);
    const auto expected = NestedStruct{
        .m = {
            .a = 12,
            .b = 3.1,
            .c = "hello world",
            .d = true,
        }};

    ASSERT_EQ(result, expected);
}

TEST(yaml_deserialization, enum_struct_simple)
{
    const auto yaml =
        R"(FruitStruct:
   f: APPLE)";
    const auto result = ufps::yaml::deserialize<FruitStruct>(yaml);
    const auto expected = FruitStruct{.f = Fruit::APPLE};

    ASSERT_EQ(result, expected);
}

TEST(yaml_deserialization, enum_struct_second_value)
{
    const auto yaml =
        R"(FruitStruct:
   f: BANANA)";
    const auto result = ufps::yaml::deserialize<FruitStruct>(yaml);
    const auto expected = FruitStruct{.f = Fruit::BANANA};

    ASSERT_EQ(result, expected);
}
