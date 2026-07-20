#include "Math.hpp"

#include <cmath>
#include <stdexcept>

namespace resonance::math {

float &Matrix4::at(std::size_t column, std::size_t row) { return elements[column * 4 + row]; }

float Matrix4::at(std::size_t column, std::size_t row) const { return elements[column * 4 + row]; }

float dot(Vector3 left, Vector3 right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vector3 cross(Vector3 left, Vector3 right) {
    return {
        .x = left.y * right.z - left.z * right.y,
        .y = left.z * right.x - left.x * right.z,
        .z = left.x * right.y - left.y * right.x,
    };
}

Vector3 normalize(Vector3 vector) {
    const float length_squared = dot(vector, vector);
    if (length_squared <= 0.0F) {
        throw std::domain_error("cannot normalize a zero-length vector");
    }

    const float inverse_length = 1.0F / std::sqrt(length_squared);
    return {
        .x = vector.x * inverse_length,
        .y = vector.y * inverse_length,
        .z = vector.z * inverse_length,
    };
}

Matrix4 multiply(const Matrix4 &left, const Matrix4 &right) {
    Matrix4 result;
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t index = 0; index < 4; ++index) {
                result.at(column, row) += left.at(index, row) * right.at(column, index);
            }
        }
    }
    return result;
}

Matrix4 look_at(Vector3 eye, Vector3 target, Vector3 up) {
    const Vector3 forward = normalize({
        .x = target.x - eye.x,
        .y = target.y - eye.y,
        .z = target.z - eye.z,
    });
    const Vector3 right = normalize(cross(forward, up));
    const Vector3 camera_up = cross(right, forward);

    Matrix4 result;
    result.at(0, 0) = right.x;
    result.at(0, 1) = camera_up.x;
    result.at(0, 2) = -forward.x;
    result.at(1, 0) = right.y;
    result.at(1, 1) = camera_up.y;
    result.at(1, 2) = -forward.y;
    result.at(2, 0) = right.z;
    result.at(2, 1) = camera_up.z;
    result.at(2, 2) = -forward.z;
    result.at(3, 0) = -dot(right, eye);
    result.at(3, 1) = -dot(camera_up, eye);
    result.at(3, 2) = dot(forward, eye);
    result.at(3, 3) = 1.0F;
    return result;
}

Matrix4 perspective(float vertical_field_of_view, float aspect_ratio, float near_plane,
                    float far_plane) {
    const float vertical_scale = 1.0F / std::tan(vertical_field_of_view * 0.5F);

    Matrix4 result;
    result.at(0, 0) = vertical_scale / aspect_ratio;
    result.at(1, 1) = vertical_scale;
    result.at(2, 2) = far_plane / (near_plane - far_plane);
    result.at(2, 3) = -1.0F;
    result.at(3, 2) = near_plane * far_plane / (near_plane - far_plane);
    return result;
}

} // namespace resonance::math
