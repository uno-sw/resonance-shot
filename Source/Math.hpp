#pragma once

#include <array>
#include <cstddef>

namespace resonance::math {

struct Vector3 {
    float x;
    float y;
    float z;
};

struct Matrix4 {
    std::array<float, 16> elements{};

    float &at(std::size_t column, std::size_t row);
    float at(std::size_t column, std::size_t row) const;
};

float dot(Vector3 left, Vector3 right);
Vector3 cross(Vector3 left, Vector3 right);
Vector3 normalize(Vector3 vector);
Matrix4 multiply(const Matrix4 &left, const Matrix4 &right);
Matrix4 look_at(Vector3 eye, Vector3 target, Vector3 up);
Matrix4 perspective(float vertical_field_of_view, float aspect_ratio, float near_plane,
                    float far_plane);

} // namespace resonance::math
