#pragma once

#include <format>
#include <string>

#include "math/matrix4.h"
#include "math/quaternion.h"
#include "math/vector3.h"

namespace ufps
{
    class Transform
    {
    public:
        constexpr Transform()
            : Transform({0.f}, {1.f}, {})
        {
        }

        // Transform(const Vector3 &position, const Vector3 &scale)
        //     : Transform(position, scale, {0.f, 0.f, 0.f, 1.f})
        // {
        // }

        constexpr Transform(const Vector3 &position, const Vector3 &scale, const Quaternion &rotation)
            : position(position),
              scale(scale),
              rotation(rotation)
        {
        }

        constexpr Transform(const Matrix4 &matrix)
            : position{matrix[12], matrix[13], matrix[14]},
              scale{},
              rotation{}
        {
            scale = Vector3{
                Vector3{matrix[0], matrix[1], matrix[2]}.length(),
                Vector3{matrix[4], matrix[5], matrix[6]}.length(),
                Vector3{matrix[8], matrix[9], matrix[10]}.length(),
            };

            auto trace = 0.f;
            auto s = 0.f;

            float scale_x = std::sqrt(matrix[0] * matrix[0] + matrix[1] * matrix[1] + matrix[2] * matrix[2]);
            float scale_y = std::sqrt(matrix[4] * matrix[4] + matrix[5] * matrix[5] + matrix[6] * matrix[6]);
            float scale_z = std::sqrt(matrix[8] * matrix[8] + matrix[9] * matrix[9] + matrix[10] * matrix[10]);

            auto m00 = matrix[0] / scale_x;
            auto m10 = matrix[1] / scale_x;
            auto m20 = matrix[2] / scale_x;

            auto m01 = matrix[4] / scale_y;
            auto m11 = matrix[5] / scale_y;
            auto m21 = matrix[6] / scale_y;

            auto m02 = matrix[8] / scale_z;
            auto m12 = matrix[9] / scale_z;
            auto m22 = matrix[10] / scale_z;

            trace = m00 + m11 + m22;

            if (trace > 0.f)
            {
                s = 0.5f / std::sqrt(trace + 1.f);
                rotation.w = 0.25f / s;
                rotation.x = (m21 - m12) * s;
                rotation.y = (m02 - m20) * s;
                rotation.z = (m10 - m01) * s;
            }
            else
            {
                if (m00 > m11 && m00 > m22)
                {
                    s = 2.f * std::sqrt(1.f + m00 - m11 - m22);
                    rotation.w = (m21 - m12) / s;
                    rotation.x = 0.25f * s;
                    rotation.y = (m01 + m10) / s;
                    rotation.z = (m02 + m20) / s;
                }
                else if (m11 > m22)
                {
                    s = 2.f * std::sqrt(1.f + m11 - m00 - m22);
                    rotation.w = (m02 - m20) / s;
                    rotation.x = (m01 + m10) / s;
                    rotation.y = 0.25f * s;
                    rotation.z = (m12 + m21) / s;
                }
                else
                {
                    s = 2.f * std::sqrt(1.f + m22 - m00 - m11);
                    rotation.w = (m10 - m01) / s;
                    rotation.x = (m02 + m20) / s;
                    rotation.y = (m12 + m21) / s;
                    rotation.z = 0.25f * s;
                }
            }
        }

        constexpr operator Matrix4() const
        {
            return Matrix4{position} * Matrix4{rotation} * Matrix4{scale, Matrix4::Scale{}};
        }

        Vector3 position;
        Vector3 scale;
        Quaternion rotation;
    };

    inline auto to_string(const Transform &transform) -> std::string
    {
        return std::format("pos: {} scale: {} rot: {}", transform.position, transform.scale, transform.rotation);
    }
}