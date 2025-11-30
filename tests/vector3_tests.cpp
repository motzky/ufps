#include <gtest/gtest.h>

#include "math/vector3.h"
#include "utils/formatter.h"

TEST(vector3, emtpy_ctor)
{
    const auto v = ufps::Vector3{};
    ASSERT_FLOAT_EQ(v.x, 0.f);
    ASSERT_FLOAT_EQ(v.y, 0.f);
    ASSERT_FLOAT_EQ(v.z, 0.f);
}

TEST(vector3, single_value_ctor)
{
    const auto v = ufps::Vector3{1.1f};
    ASSERT_FLOAT_EQ(v.x, 1.1f);
    ASSERT_FLOAT_EQ(v.y, 1.1f);
    ASSERT_FLOAT_EQ(v.z, 1.1f);
}

TEST(vector3, all_components_ctor)
{
    const auto v = ufps::Vector3{1.1f, 2.2f, 3.3f};
    ASSERT_FLOAT_EQ(v.x, 1.1f);
    ASSERT_FLOAT_EQ(v.y, 2.2f);
    ASSERT_FLOAT_EQ(v.z, 3.3f);
}

TEST(vector3, negate)
{
    const auto v1 = ufps::Vector3{1.f, 2.f, 3.f};
    const auto expected = ufps::Vector3{-1.f, -2.f, -3.f};

    ASSERT_EQ(-v1, expected);
}

TEST(vector3, normalize)
{
    auto v1 = ufps::Vector3{1.f, 2.f, 3.f};
    auto norm = ufps::Vector3::normalize(v1);

    const auto expected = ufps::Vector3{0.26726124f, 0.5345225f, 0.80178374f};

    ASSERT_EQ(norm, expected);
}

TEST(vector3, normalize_point)
{
    auto v1 = ufps::Vector3{};
    auto norm = ufps::Vector3::normalize(v1);

    const auto expected = ufps::Vector3{};

    ASSERT_EQ(norm, expected);
}

TEST(vector3, cross_x_axis_cross_y_axis)
{
    const auto v1 = ufps::Vector3{1.f, 0.f, 0.f};
    const auto v2 = ufps::Vector3{0.f, 1.f, 0.f};

    const auto expected = ufps::Vector3{0.f, 0.f, 1.f};

    const auto c = ufps::Vector3::cross(v1, v2);

    ASSERT_EQ(c, expected);
}

TEST(vector3, cross)
{
    const auto v1 = ufps::Vector3{1.f, 2.f, 3.f};
    const auto v2 = ufps::Vector3{4.f, 5.f, 6.f};

    const auto expected = ufps::Vector3{-3.f, 6.f, -3.f};

    const auto c = ufps::Vector3::cross(v1, v2);

    ASSERT_EQ(c, expected);
}

TEST(vector3, multiply_assign_scalar)
{
    auto v1 = ufps::Vector3{1.f, 2.f, 3.f};

    const auto expected = ufps::Vector3{10.f, 20.f, 30.f};

    v1 *= 10.f;

    ASSERT_EQ(v1, expected);
}

TEST(vector3, add_zero)
{
    auto v1 = ufps::Vector3{1.f, 2.f, 3.f};
    const auto zero = ufps::Vector3{};
    const auto expected = ufps::Vector3{1.f, 2.f, 3.f};

    v1 += zero;

    ASSERT_EQ(v1, expected);
}

TEST(vector3, subtract_negated)
{
    const auto v1 = ufps::Vector3{1.f, 2.f, 3.f};
    const auto v2 = v1 + (-v1);
    const auto zero = ufps::Vector3{};

    ASSERT_EQ(v2, zero);
}

TEST(vector3, subtract)
{
    const auto v1 = ufps::Vector3{10.f, 20.f, 30.f};
    const auto v2 = ufps::Vector3{4.f, 5.f, 6.4f};
    const auto expected = ufps::Vector3{6.f, 15.f, 23.6f};

    auto v3 = v1 - v2;

    ASSERT_EQ(v3.x, expected.x);
    ASSERT_EQ(v3.y, expected.y);
    ASSERT_EQ(v3.z, expected.z);
}
TEST(vector3, dot_product_basic)
{
    const auto v1 = ufps::Vector3{1.f, 2.f, 3.f};
    const auto v2 = ufps::Vector3{4.f, -5.f, 6.f};
    const auto expected = 12.f; // 1*4 + 2*(-5) + 3*6 = 12

    const auto dot = ufps::Vector3::dot(v1, v2);

    ASSERT_FLOAT_EQ(dot, expected);
}

TEST(vector3, dot_product_with_zero)
{
    const auto v1 = ufps::Vector3{1.f, 2.f, 3.f};
    const auto zero = ufps::Vector3{};
    const auto expected = 0.f;

    const auto dot = ufps::Vector3::dot(v1, zero);

    ASSERT_FLOAT_EQ(dot, expected);
}

TEST(vector3, dot_product_with_self)
{
    const auto v1 = ufps::Vector3{2.f, 3.f, 6.f};
    const auto expected = 2.f * 2.f + 3.f * 3.f + 6.f * 6.f; // 4 + 9 + 36 = 49

    const auto dot = ufps::Vector3::dot(v1, v1);

    ASSERT_FLOAT_EQ(dot, expected);
}

TEST(vector3, to_string)
{
    const auto v = ufps::Vector3{1.f, 2.f, 3.f};
    const auto str = v.to_string();

    ASSERT_EQ(str, "x=1 y=2 z=3");
}

TEST(vector3, format)
{
    const auto v = ufps::Vector3{1.f, 2.f, 3.f};
    const auto str = std::format("{}", v);

    ASSERT_EQ(str, "x=1 y=2 z=3");
}