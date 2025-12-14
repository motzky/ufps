#include <gtest/gtest.h>

#include <cmath>
#include <numbers>
#include <print>

#include "math/matrix4.h"
#include "math/quaternion.h"

TEST(quaternion, multiply)
{
    auto angle = std::numbers::pi_v<float> / 2.f;

    auto q1 = ufps::Quaternion(std::sin(angle / 2.f), 0.f, 0.f, std::cos(angle / 2.f));
    auto q2 = ufps::Quaternion(0.f, std::sin(angle / 2.f), 0.f, std::cos(angle / 2.f));

    auto result = q1 * q2;

    auto expected = ufps::Quaternion(.5f, .5f, .5f, .5f);
    ASSERT_FLOAT_EQ(result.x, expected.x);
    ASSERT_FLOAT_EQ(result.y, expected.y);
    ASSERT_FLOAT_EQ(result.z, expected.z);
    ASSERT_FLOAT_EQ(result.w, expected.w);
}

TEST(quaternion, multiply2)
{
    auto q1 = ufps::Quaternion{0.64207685f, 0.37862414f, 0.29617387f, 0.597212f};
    auto q2 = ufps::Quaternion{0.7071f, 0.f, 0.f, 0.7071f};

    auto result = q1 * q2;

    ASSERT_FLOAT_EQ(result.x, 0.876301f);
    ASSERT_FLOAT_EQ(result.y, 0.47714967f);
    ASSERT_FLOAT_EQ(result.z, -0.05830057f);
    ASSERT_FLOAT_EQ(result.w, -0.031723917f);
}

TEST(quaternion, multiply_inv)
{
    auto angle = std::numbers::pi_v<float> / 2.f;

    auto q1 = ufps::Quaternion(std::sin(angle / 2.f), 0.f, 0.f, std::cos(angle / 2.f));
    auto q2 = ufps::Quaternion(0.f, std::sin(angle / 2.f), 0.f, std::cos(angle / 2.f));

    auto result = q2 * q1;

    auto expected = ufps::Quaternion(.5f, .5f, -.5f, .5f);
    ASSERT_FLOAT_EQ(result.x, expected.x);
    ASSERT_FLOAT_EQ(result.y, expected.y);
    ASSERT_FLOAT_EQ(result.z, expected.z);
    ASSERT_FLOAT_EQ(result.w, expected.w);
}
