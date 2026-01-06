#include <gtest/gtest.h>

#include <numbers>
#include <print>

#include "math/matrix3.h"
#include "math/vector3.h"

TEST(matrix3, ctor_identity)
{
    float vals[] = {
        1.f,
        0.f,
        0.f,
        0.f,
        1.f,
        0.f,
        0.f,
        0.f,
        1.f,
    };
    const auto m = ufps::Matrix3{};

    ASSERT_FLOAT_EQ(m[0], vals[0]);
    ASSERT_FLOAT_EQ(m[1], vals[1]);
    ASSERT_FLOAT_EQ(m[2], vals[2]);
    ASSERT_FLOAT_EQ(m[3], vals[3]);
    ASSERT_FLOAT_EQ(m[4], vals[4]);
    ASSERT_FLOAT_EQ(m[5], vals[5]);
    ASSERT_FLOAT_EQ(m[6], vals[6]);
    ASSERT_FLOAT_EQ(m[7], vals[7]);
    ASSERT_FLOAT_EQ(m[8], vals[8]);
}

TEST(matrix3, ctor_from_three_vectors)
{
    const auto v1 = ufps::Vector3{3.f, 2.f, 1.f};
    const auto v2 = ufps::Vector3{7.f, 8.f, 9.f};
    const auto v3 = ufps::Vector3{4.f, 5.f, 6.f};

    const auto m = ufps::Matrix3(v1, v2, v3);

    ASSERT_FLOAT_EQ(m[0], 3.f);
    ASSERT_FLOAT_EQ(m[1], 2.f);
    ASSERT_FLOAT_EQ(m[2], 1.f);
    ASSERT_FLOAT_EQ(m[3], 7.f);
    ASSERT_FLOAT_EQ(m[4], 8.f);
    ASSERT_FLOAT_EQ(m[5], 9.f);
    ASSERT_FLOAT_EQ(m[6], 4.f);
    ASSERT_FLOAT_EQ(m[7], 5.f);
    ASSERT_FLOAT_EQ(m[8], 6.f);
}

TEST(matrix3, multiply_by_identity)
{
    float vals[] = {
        1.f,
        2.f,
        3.f,
        1.f,
        2.f,
        3.f,
        1.f,
        2.f,
        3.f,
    };
    const auto m1 = ufps::Matrix3{vals};
    const auto m2 = ufps::Matrix3{};

    ASSERT_EQ(m1 * m2, m1);
}

TEST(matrix3, multiply_by_matrix)
{
    const auto m1 = ufps::Matrix3{{0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f}};
    const auto m2 = ufps::Matrix3{{0.5f, 1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f, 8.5f}};

    const auto expected = ufps::Matrix3{
        {19.75f, 24.f, 28.5f, 48.25f, 60.f, 73.5f, 76.75f, 96.f, 118.5f}};

    ASSERT_EQ(m1 * m2, expected);
}

TEST(matrix3, multiply_with_vector)
{
    const auto v1 = ufps::Vector3{1.f, 2.f, 3.f};
    const auto v2 = ufps::Vector3{4.f, 5.f, 6.f};
    const auto v3 = ufps::Vector3{7.f, 8.f, 9.f};

    const auto m = ufps::Matrix3(v1, v2, v3);
    const auto v4 = ufps::Vector3{10.f, 10.f, 10.f};

    const auto mult = m * v4;

    ASSERT_FLOAT_EQ(mult.x, 120.f);
    ASSERT_FLOAT_EQ(mult.y, 150.f);
    ASSERT_FLOAT_EQ(mult.z, 180.f);
}

TEST(matrix3, determinate)
{
    float vals[] = {
        1.f, 2.f, -1.f,
        2.f, 1.f, 2.f,
        -1.f, 2.f, 1.f};

    float expected = -16.f;

    const auto m = ufps::Matrix3{vals};

    const auto det = ufps::Matrix3::determinate(m);

    ASSERT_FLOAT_EQ(det, expected);
}

TEST(matrix3, invert)
{
    float vals[] = {
        1.f, 2.f, -1.f,
        2.f, 1.f, 2.f,
        -1.f, 2.f, 1.f};

    float expected_vals[] = {
        3.f / 16.f, .25f, -5.f / 16.f,
        .25f, 0.f, .25f,
        -5.f / 16.f, .25f, 3.f / 16.f};

    const auto m = ufps::Matrix3{vals};

    const auto m_inv = ufps::Matrix3::invert(m);

    ASSERT_FLOAT_EQ(m_inv[0], expected_vals[0]);
    ASSERT_FLOAT_EQ(m_inv[1], expected_vals[1]);
    ASSERT_FLOAT_EQ(m_inv[2], expected_vals[2]);
    ASSERT_FLOAT_EQ(m_inv[3], expected_vals[3]);
    ASSERT_FLOAT_EQ(m_inv[4], expected_vals[4]);
    ASSERT_FLOAT_EQ(m_inv[5], expected_vals[5]);
    ASSERT_FLOAT_EQ(m_inv[6], expected_vals[6]);
    ASSERT_FLOAT_EQ(m_inv[7], expected_vals[7]);
    ASSERT_FLOAT_EQ(m_inv[8], expected_vals[8]);
}

TEST(matrix3, matrix_times_invert_equals_identiy)
{
    float vals[] = {
        1.f, 2.f, -1.f,
        2.f, 1.f, 2.f,
        -1.f, 2.f, 1.f};

    const auto identity = ufps::Matrix3{};

    const auto m = ufps::Matrix3{vals};

    const auto m_inv = ufps::Matrix3::invert(m);

    const auto result = m * m_inv;

    ASSERT_FLOAT_EQ(result[0], identity[0]);
    ASSERT_FLOAT_EQ(result[1], identity[1]);
    ASSERT_FLOAT_EQ(result[2], identity[2]);
    ASSERT_FLOAT_EQ(result[3], identity[3]);
    ASSERT_FLOAT_EQ(result[4], identity[4]);
    ASSERT_FLOAT_EQ(result[5], identity[5]);
    ASSERT_FLOAT_EQ(result[6], identity[6]);
    ASSERT_FLOAT_EQ(result[7], identity[7]);
    ASSERT_FLOAT_EQ(result[8], identity[8]);
}