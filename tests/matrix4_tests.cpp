#include <gtest/gtest.h>

#include <cstring>
#include <numbers>
#include <print>

#include "math/matrix4.h"
#include "math/vector3.h"

TEST(matrix4, identity_ctor)
{
    const auto m = ufps::Matrix4{};
    const auto expected = std::array<float, 16u>{
        1.f,
        0.f,
        0.f,
        0.f,
        0.f,
        1.f,
        0.f,
        0.f,
        0.f,
        0.f,
        1.f,
        0.f,
        0.f,
        0.f,
        0.f,
        1.f,
    };

    const auto result = m.data();
    ASSERT_EQ(result.size(), expected.size());
    EXPECT_EQ(std::memcmp(result.data(), expected.data(), result.size_bytes()), 0);
}

TEST(matrix4, element_ctor)
{
    const auto expected = std::array<float, 16u>{
        1.f,
        2.f,
        3.f,
        4.f,
        5.f,
        6.f,
        7.f,
        8.f,
        9.f,
        10.f,
        11.f,
        12.f,
        13.f,
        14.f,
        15.f,
        16.f,
    };

    const auto m = ufps::Matrix4{expected};

    const auto result = m.data();
    ASSERT_EQ(result.size(), expected.size());
    EXPECT_EQ(std::memcmp(result.data(), expected.data(), result.size_bytes()), 0);
}

TEST(matrix4, incorrect_element_size_ctor)
{
    const auto expected = std::array<float, 161u>{
        1.f,
    };

    EXPECT_THROW(ufps::Matrix4{expected}, ufps::Exception);
}

TEST(matrix4, equals_operator)
{
    const auto m1 = ufps::Matrix4{};

    ASSERT_TRUE(m1 == m1);
}

TEST(matrix4, multiply_by_identity)
{
    const auto m1 = ufps::Matrix4{{
        1.f,
        2.f,
        3.f,
        4.f,
        1.f,
        2.f,
        3.f,
        4.f,
        1.f,
        2.f,
        3.f,
        4.f,
        1.f,
        2.f,
        3.f,
        4.f,
    }};
    const auto m2 = ufps::Matrix4{};

    ASSERT_EQ(m1 * m2, m1);
}

TEST(matrix4, multiply)
{
    const auto m1 = ufps::Matrix4{{1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f,
                                   11.f, 12.f, 13.f, 14.f, 15.f, 16.f}};

    const auto m2 = ufps::Matrix4{{1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f, 8.5f, 9.5f, 10.5f,
                                   11.5f, 12.5f, 13.5f, 14.5f, 15.5f, 16.5f}};

    const auto expected = ufps::Matrix4{{104.f, 116.f, 128.f, 140.f, 216.f, 244.f, 272.f, 300.f, 328.f, 372.f, 416.f, 460.f, 440.f, 500.f, 560.f, 620.f}};

    ASSERT_EQ(m1 * m2, expected);
}
TEST(matrix4, multiply_assing)
{
    auto m = ufps::Matrix4{{1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f,
                            11.f, 12.f, 13.f, 14.f, 15.f, 16.f}};

    m *= ufps::Matrix4{{1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f, 8.5f, 9.5f, 10.5f,
                        11.5f, 12.5f, 13.5f, 14.5f, 15.5f, 16.5f}};

    const auto expected = ufps::Matrix4{{104.f, 116.f, 128.f, 140.f, 216.f, 244.f, 272.f, 300.f, 328.f, 372.f, 416.f, 460.f, 440.f, 500.f, 560.f, 620.f}};

    ASSERT_EQ(m, expected);
}

TEST(matrix4, look_at)
{
    const auto view = ufps::Matrix4::look_at({1.f, 0.f, 5.f}, {0.f}, {0.f, 1.f, 0.f});
    float vals[] = {0.980581f, 0.f, 0.196116f, 0.f,
                    0.f, 1.f, 0.f, 0.f,
                    -0.196116f, 0.f, 0.980581f, 0.f,
                    0.f, 0.f, -5.099020f, 1.f};
    const auto expected = ufps::Matrix4{vals};

    const auto view_spn = view.data();
    const auto exp_spn = expected.data();
    for (auto i = 0u; i < 16u; i++)
    {
        ASSERT_NEAR(view_spn[i], exp_spn[i], 0.00001f);
    }
}

TEST(matrix4, perspective)
{
    const auto proj = ufps::Matrix4::perspective(std::numbers::pi_v<float> / 4.f, 800.f, 600.f, 0.1f, 100.f);
    float vals[] = {1.81066f, 0.f, 0.f, 0.f,
                    0.f, 2.414213f, 0.f, 0.f,
                    0.f, 0.f, -1.002002f, -1.f,
                    0.f, 0.f, -0.200200f, 0.f};
    const auto expected = ufps::Matrix4{vals};

    const auto view_spn = proj.data();
    const auto exp_spn = expected.data();
    for (auto i = 0u; i < 16u; i++)
    {
        ASSERT_NEAR(view_spn[i], exp_spn[i], 0.00001f);
    }
}

TEST(matrix4, invert)
{

    const auto m = ufps::Matrix4{
        {0.6f, 2.4f, 1.1f, 0.0f, 2.4f, 0.6f, -0.4f, 1.0f, 1.1f, -0.4f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}};

    const auto inv = ufps::Matrix4::invert(m);

    const auto expected = ufps::Matrix4{

        {-0.0323939,
         0.304503,
         0.262391,
         -0.304503,
         0.304503,
         0.137674,
         -0.466472,
         -0.137674,
         0.262391,
         -0.466472,
         0.874636,
         0.466472,
         0,
         0,
         0,
         1}};

    const auto inv_spn = inv.data();
    const auto expected_spn = expected.data();

    for (auto i = 0u; i < 16u; ++i)
    {
        ASSERT_NEAR(inv_spn[i], expected_spn[i], 0.001f);
    }
}

TEST(matrix4, mul_vector4)
{
    const auto m = ufps::Matrix4{
        {1.f,
         2.f,
         3.f,
         4.f,
         5.f,
         6.f,
         7.f,
         8.f,
         9.f,
         10.f,
         11.f,
         12.f,
         13.f,
         14.f,
         15.f,
         16.f}};

    const auto v = ufps::Vector4{.5f, 1.5f, 2.5f, 3.5f};

    const auto result = m * v;
    std::println("{}", result);

    const auto expected = ufps::Vector4{76.f, 84.f, 92.f, 100.f};

    ASSERT_EQ(result, expected);
}
