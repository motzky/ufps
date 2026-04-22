#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "serialization/yaml_serializer.h"

struct Simple
{
    int a;
};

struct MultiMember
{
    int a;
    float b;
    std::string c;
    bool d;
};

struct Array
{
    std::vector<int> a;
};

struct Map
{
    std::unordered_map<std::string, int> a;
};

TEST(yaml_serialization, simple_struct)
{
    const auto result = ufps::yaml::serialize(Simple{.a = 12});

    const auto expected = R"(Simple:
a:12)";

    ASSERT_EQ(result, expected);
}
