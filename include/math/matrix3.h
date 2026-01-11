#pragma once

#include <array>
#include <cstring>
#include <format>
#include <initializer_list>
#include <ranges>
#include <span>

#include "math/quaternion.h"
#include "math/vector3.h"
#include "math/vector4.h"
#include "utils/ensure.h"

namespace ufps
{
    class Matrix3
    {
    public:
        struct Scale
        {
        };
        // initialize to identity
        constexpr Matrix3()
            : _elements({1.f,
                         0.f,
                         0.f,
                         0.f,
                         1.f,
                         0.f,
                         0.f,
                         0.f,
                         1.f})
        {
        }

        constexpr explicit Matrix3(std::initializer_list<float> il)
            : Matrix3(std::span<const float>{std::move(il)})
        {
        }

        constexpr Matrix3(const std::array<float, 9u> &elements)
            : _elements{elements}
        {
        }

        constexpr explicit Matrix3(const std::span<const float> elements)
            : Matrix3{}
        {
            ensure(elements.size() == 9u, "require exactly 9 elements");
            std::ranges::copy(elements, std::ranges::begin(_elements));
        }

        constexpr Matrix3(const Vector3 &v1, const Vector3 &v2, const Vector3 &v3)
            : _elements(
                  {v1.x, v1.y, v1.z,
                   v2.x, v2.y, v2.z,
                   v3.x, v3.y, v3.z})
        {
        }

        constexpr auto data() const -> std::span<const float>
        {
            return _elements;
        }

        constexpr auto operator[](this auto &&self, std::size_t index) -> auto &
        {
            return self._elements[index];
        }

        static constexpr auto transpose(Matrix3 m) -> Matrix3
        {
            std::ranges::swap(m[1], m[3]);
            std::ranges::swap(m[2], m[6]);
            std::ranges::swap(m[5], m[7]);

            return m;
        }

        auto row(std::size_t index) const -> Vector3
        {
            ensure(index < 3, "index out of range");

            return {_elements[index], _elements[index + 3u], _elements[index + 6u]};
        }

        friend constexpr auto operator*=(Matrix3 &m1, const Matrix3 &m2) -> Matrix3 &;

        constexpr auto operator==(const Matrix3 &) const -> bool = default;

        static constexpr auto determinate(const Matrix3 &m) -> float
        {
            return m[0] * m[4] * m[8] +
                   m[1] * m[5] * m[6] +
                   m[2] * m[3] * m[7] -

                   m[0] * m[5] * m[7] -
                   m[1] * m[3] * m[8] -
                   m[2] * m[4] * m[6];
        }

        static constexpr auto invert(const Matrix3 &m) -> Matrix3
        {
            const auto adjoint = Matrix3{{(m[4] * m[8]) - (m[5] * m[7]),
                                          -(m[1] * m[8]) + (m[2] * m[7]),
                                          (m[1] * m[5]) - (m[2] * m[4]),

                                          -(m[3] * m[8]) + (m[5] * m[6]),
                                          (m[0] * m[8]) - (m[2] * m[6]),
                                          -(m[0] * m[5]) + (m[2] * m[3]),

                                          (m[3] * m[7]) - (m[4] * m[6]),
                                          -(m[0] * m[7]) + (m[1] * m[6]),
                                          (m[0] * m[4]) - (m[1] * m[3])}};

            const auto det = determinate(m);

            const auto inv_vals = adjoint.data() | std::views::transform([det](auto e)
                                                                         { return e / det; });
            auto inv_arr = std::array<float, 9u>{};
            std::ranges::copy(inv_vals, std::ranges::begin(inv_arr));

            return {inv_arr};
        }

        auto to_string() const -> std::string;

    private:
        std::array<float, 9u>
            _elements;
    };

    constexpr auto operator*=(Matrix3 &m1, const Matrix3 &m2) -> Matrix3 &
    {
        auto result = Matrix3{};
        for (auto i = 0u; i < 3u; i++)
        {
            for (auto j = 0u; j < 3u; j++)
            {
                auto sum = 0.f;
                for (auto k = 0u; k < 3u; ++k)
                {
                    sum += m1._elements[i + k * 3] * m2._elements[k + j * 3];
                }
                result._elements[i + j * 3] = sum;
            }
        }

        m1 = result;
        return m1;
    }

    constexpr auto operator*(const Matrix3 &m1, const Matrix3 &m2) -> Matrix3
    {
        auto tmp{m1};
        return tmp *= m2;
    }

    constexpr auto operator*(const Matrix3 &m, const Vector3 &v) -> Vector3
    {
        return {m[0] * v.x + m[3] * v.y + m[6] * v.z,
                m[1] * v.x + m[4] * v.y + m[7] * v.z,
                m[2] * v.x + m[5] * v.y + m[8] * v.z};
    }

    inline auto Matrix3::to_string() const -> std::string
    {
        const auto *d = data().data();
        return std::format("{} {} {}\n{} {} {}\n{} {} {}",
                           d[0], d[3], d[6],
                           d[1], d[4], d[7],
                           d[2], d[5], d[8]);
    }
}
