#include <metal_stdlib>

using namespace metal;

struct CameraUniforms {
    float4x4 view_projection;
};

struct VertexOutput {
    float4 position [[position]];
    float2 floor_position;
};

vertex VertexOutput floor_vertex(uint vertex_id [[vertex_id]],
                                 constant CameraUniforms &camera [[buffer(0)]]) {
    const float2 positions[] = {
        float2(-12.0, -15.0), float2(-12.0, 5.0), float2(12.0, 5.0),
        float2(-12.0, -15.0), float2(12.0, 5.0),  float2(12.0, -15.0),
    };

    const float2 floor_position = positions[vertex_id];

    VertexOutput output;
    output.position = camera.view_projection * float4(floor_position.x, 0.0, floor_position.y, 1.0);
    output.floor_position = floor_position;
    return output;
}

fragment float4 floor_fragment(VertexOutput input [[stage_in]]) {
    const float2 cell_position = fract(input.floor_position);
    const float2 grid_distance = min(cell_position, 1.0 - cell_position);
    const bool is_grid = grid_distance.x < 0.035 || grid_distance.y < 0.035;

    const float distance_to_edge =
        min(min(input.floor_position.x + 12.0, 12.0 - input.floor_position.x),
            min(input.floor_position.y + 15.0, 5.0 - input.floor_position.y));
    const bool is_outline = distance_to_edge < 0.12;

    const float4 floor_color = float4(0.015, 0.045, 0.08, 1.0);
    const float4 grid_color = float4(0.0, 0.28, 0.46, 1.0);
    const float4 outline_color = float4(0.1, 0.8, 1.0, 1.0);

    if (is_outline) {
        return outline_color;
    }
    if (is_grid) {
        return grid_color;
    }
    return floor_color;
}

constant uint capsule_radial_segments = 24;
constant uint capsule_bottom_hemisphere_segments = 4;
constant uint capsule_cylinder_segments = 1;
constant uint capsule_top_hemisphere_segments = 4;
constant uint capsule_vertical_segments = capsule_bottom_hemisphere_segments +
                                          capsule_cylinder_segments +
                                          capsule_top_hemisphere_segments;

float3 capsule_position(uint ring, uint radial_segment) {
    constexpr float pi = 3.14159265358979323846;
    constexpr float radius = 0.6;
    constexpr float cylinder_height = 1.2;
    constexpr float3 world_position = float3(0.0, 0.0, -4.0);

    const float angle = 2.0 * pi * float(radial_segment) / float(capsule_radial_segments);
    float ring_radius = radius;
    float height = radius;

    if (ring <= capsule_bottom_hemisphere_segments) {
        const float latitude =
            -0.5 * pi + 0.5 * pi * float(ring) / float(capsule_bottom_hemisphere_segments);
        ring_radius = radius * cos(latitude);
        height = radius + radius * sin(latitude);
    } else if (ring <= capsule_bottom_hemisphere_segments + capsule_cylinder_segments) {
        const float cylinder_step =
            float(ring - capsule_bottom_hemisphere_segments) / float(capsule_cylinder_segments);
        height = radius + cylinder_height * cylinder_step;
    } else {
        const uint top_ring = ring - capsule_bottom_hemisphere_segments - capsule_cylinder_segments;
        const float latitude = 0.5 * pi * float(top_ring) / float(capsule_top_hemisphere_segments);
        ring_radius = radius * cos(latitude);
        height = radius + cylinder_height + radius * sin(latitude);
    }

    return world_position + float3(ring_radius * cos(angle), height, ring_radius * sin(angle));
}

vertex VertexOutput capsule_vertex(uint vertex_id [[vertex_id]],
                                   constant CameraUniforms &camera [[buffer(0)]]) {
    const uint triangle_vertex = vertex_id % 6;
    const uint quad = vertex_id / 6;
    const uint ring = quad / capsule_radial_segments;
    const uint radial_segment = quad % capsule_radial_segments;

    const uint ring_offsets[] = {0, 1, 1, 0, 1, 0};
    const uint radial_offsets[] = {0, 0, 1, 0, 1, 1};
    const float3 position = capsule_position(ring + ring_offsets[triangle_vertex],
                                             (radial_segment + radial_offsets[triangle_vertex]) %
                                                 capsule_radial_segments);

    VertexOutput output;
    output.position = camera.view_projection * float4(position, 1.0);
    output.floor_position = float2(0.0);
    return output;
}

fragment float4 capsule_fragment(VertexOutput input [[stage_in]]) {
    return float4(0.95, 0.28, 0.18, 1.0);
}
