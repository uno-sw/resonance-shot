#include "Math.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>

namespace {

constexpr float tolerance = 0.00001F;
int failure_count = 0;

void expect_near(float actual, float expected, std::string_view description) {
    if (std::abs(actual - expected) <= tolerance) {
        return;
    }

    std::cerr << description << ": expected " << expected << ", got " << actual << '\n';
    ++failure_count;
}

void expect_vector(resonance::math::Vector3 actual, resonance::math::Vector3 expected,
                   std::string_view description) {
    expect_near(actual.x, expected.x, std::string(description) + " x");
    expect_near(actual.y, expected.y, std::string(description) + " y");
    expect_near(actual.z, expected.z, std::string(description) + " z");
}

resonance::math::Matrix4 matrix_from_rows(const std::array<float, 16> &values) {
    resonance::math::Matrix4 matrix;
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            matrix.at(column, row) = values[row * 4 + column];
        }
    }
    return matrix;
}

void expect_matrix(const resonance::math::Matrix4 &actual, const std::array<float, 16> &expected,
                   std::string_view description) {
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            expect_near(actual.at(column, row), expected[row * 4 + column], description);
        }
    }
}

void test_vector_operations() {
    const resonance::math::Vector3 left{.x = 1.0F, .y = 2.0F, .z = 3.0F};
    const resonance::math::Vector3 right{.x = 4.0F, .y = -5.0F, .z = 6.0F};
    expect_near(resonance::math::dot(left, right), 12.0F, "dot product");

    const resonance::math::Vector3 cross = resonance::math::cross(
        {.x = 1.0F, .y = 0.0F, .z = 0.0F}, {.x = 0.0F, .y = 1.0F, .z = 0.0F});
    expect_vector(cross, {.x = 0.0F, .y = 0.0F, .z = 1.0F}, "cross product");
    expect_near(resonance::math::dot(cross, {.x = 1.0F, .y = 0.0F, .z = 0.0F}), 0.0F,
                "cross product orthogonal to left");
    expect_near(resonance::math::dot(cross, {.x = 0.0F, .y = 1.0F, .z = 0.0F}), 0.0F,
                "cross product orthogonal to right");

    expect_vector(resonance::math::normalize({.x = 3.0F, .y = 4.0F, .z = 0.0F}),
                  {.x = 0.6F, .y = 0.8F, .z = 0.0F}, "normalized vector");
}

void test_matrix_storage_and_multiplication() {
    resonance::math::Matrix4 storage;
    storage.at(2, 1) = 42.0F;
    expect_near(storage.elements[9], 42.0F, "column-major storage");

    const resonance::math::Matrix4 identity = matrix_from_rows({
        1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F,
    });
    const resonance::math::Matrix4 left = matrix_from_rows({
        1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F,
        9.0F, 10.0F, 11.0F, 12.0F, 13.0F, 14.0F, 15.0F, 16.0F,
    });
    expect_matrix(resonance::math::multiply(left, identity),
                  {1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F,
                   9.0F, 10.0F, 11.0F, 12.0F, 13.0F, 14.0F, 15.0F, 16.0F},
                  "identity multiplication");

    const resonance::math::Matrix4 right = matrix_from_rows({
        17.0F, 18.0F, 19.0F, 20.0F, 21.0F, 22.0F, 23.0F, 24.0F,
        25.0F, 26.0F, 27.0F, 28.0F, 29.0F, 30.0F, 31.0F, 32.0F,
    });
    expect_matrix(resonance::math::multiply(left, right),
                  {250.0F, 260.0F, 270.0F, 280.0F, 618.0F, 644.0F, 670.0F, 696.0F,
                   986.0F, 1028.0F, 1070.0F, 1112.0F, 1354.0F, 1412.0F, 1470.0F,
                   1528.0F},
                  "matrix multiplication");
}

void test_view_and_projection_matrices() {
    const resonance::math::Matrix4 view = resonance::math::look_at(
        {.x = 0.0F, .y = 0.0F, .z = 1.0F}, {.x = 0.0F, .y = 0.0F, .z = 0.0F},
        {.x = 0.0F, .y = 1.0F, .z = 0.0F});
    expect_matrix(view,
                  {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
                   0.0F, 0.0F, 1.0F, -1.0F, 0.0F, 0.0F, 0.0F, 1.0F},
                  "look-at matrix");

    constexpr float pi = 3.14159265358979323846F;
    const resonance::math::Matrix4 projection =
        resonance::math::perspective(pi / 2.0F, 2.0F, 1.0F, 11.0F);
    expect_matrix(projection,
                  {0.5F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
                   0.0F, 0.0F, -1.1F, -1.1F, 0.0F, 0.0F, -1.0F, 0.0F},
                  "perspective matrix");
}

} // namespace

int main() {
    test_vector_operations();
    test_matrix_storage_and_multiplication();
    test_view_and_projection_matrices();

    if (failure_count != 0) {
        std::cerr << failure_count << " math test assertion(s) failed\n";
        return 1;
    }

    return 0;
}
