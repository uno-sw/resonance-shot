#include <metal_stdlib>

using namespace metal;

struct CameraUniforms {
    float4x4 view_projection;
};

struct LightingUniforms {
    float4 direction;
    float4 directional_color;
    float4 ambient_color;
};

struct FloorVertexInput {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float4 color [[attribute(2)]];
};

struct VertexOutput {
    float4 position [[position]];
    float3 normal;
    float4 color;
};

vertex VertexOutput floor_vertex(FloorVertexInput input [[stage_in]],
                                 constant CameraUniforms &camera [[buffer(0)]]) {
    VertexOutput output;
    output.position = camera.view_projection * float4(input.position, 1.0);
    output.normal = input.normal;
    output.color = input.color;
    return output;
}

float4 apply_lighting(VertexOutput input, constant LightingUniforms &lighting) {
    const float direction_length_squared = dot(lighting.direction.xyz, lighting.direction.xyz);
    float diffuse = 0.0;
    if (direction_length_squared > 0.0) {
        const float3 direction = lighting.direction.xyz * rsqrt(direction_length_squared);
        diffuse = max(dot(normalize(input.normal), -direction), 0.0);
    }

    const float3 light = lighting.ambient_color.rgb + lighting.directional_color.rgb * diffuse;
    return float4(input.color.rgb * light, input.color.a);
}

fragment float4 floor_fragment(VertexOutput input [[stage_in]],
                               constant LightingUniforms &lighting [[buffer(0)]]) {
    return apply_lighting(input, lighting);
}

constant uint capsule_radial_segments = 24;
constant uint capsule_bottom_hemisphere_segments = 4;
constant uint capsule_cylinder_segments = 1;
constant uint capsule_top_hemisphere_segments = 4;
constant uint capsule_vertical_segments = capsule_bottom_hemisphere_segments +
                                          capsule_cylinder_segments +
                                          capsule_top_hemisphere_segments;

struct CapsuleGeometry {
    float3 position;
    float3 normal;
};

CapsuleGeometry capsule_geometry(uint ring, uint radial_segment) {
    constexpr float pi = 3.14159265358979323846;
    constexpr float radius = 0.6;
    constexpr float cylinder_height = 1.2;
    constexpr float3 world_position = float3(0.0, 0.0, -4.0);

    const float angle = 2.0 * pi * float(radial_segment) / float(capsule_radial_segments);
    float ring_radius = radius;
    float height = radius;
    float3 normal = float3(cos(angle), 0.0, sin(angle));

    if (ring <= capsule_bottom_hemisphere_segments) {
        const float latitude =
            -0.5 * pi + 0.5 * pi * float(ring) / float(capsule_bottom_hemisphere_segments);
        ring_radius = radius * cos(latitude);
        height = radius + radius * sin(latitude);
        normal = float3(cos(latitude) * cos(angle), sin(latitude), cos(latitude) * sin(angle));
    } else if (ring <= capsule_bottom_hemisphere_segments + capsule_cylinder_segments) {
        const float cylinder_step =
            float(ring - capsule_bottom_hemisphere_segments) / float(capsule_cylinder_segments);
        height = radius + cylinder_height * cylinder_step;
    } else {
        const uint top_ring = ring - capsule_bottom_hemisphere_segments - capsule_cylinder_segments;
        const float latitude = 0.5 * pi * float(top_ring) / float(capsule_top_hemisphere_segments);
        ring_radius = radius * cos(latitude);
        height = radius + cylinder_height + radius * sin(latitude);
        normal = float3(cos(latitude) * cos(angle), sin(latitude), cos(latitude) * sin(angle));
    }

    return {
        world_position + float3(ring_radius * cos(angle), height, ring_radius * sin(angle)),
        normal,
    };
}

vertex VertexOutput capsule_vertex(uint vertex_id [[vertex_id]],
                                   constant CameraUniforms &camera [[buffer(0)]]) {
    const uint triangle_vertex = vertex_id % 6;
    const uint quad = vertex_id / 6;
    const uint ring = quad / capsule_radial_segments;
    const uint radial_segment = quad % capsule_radial_segments;

    const uint ring_offsets[] = {0, 1, 1, 0, 1, 0};
    const uint radial_offsets[] = {0, 0, 1, 0, 1, 1};
    const CapsuleGeometry geometry = capsule_geometry(
        ring + ring_offsets[triangle_vertex],
        (radial_segment + radial_offsets[triangle_vertex]) % capsule_radial_segments);

    VertexOutput output;
    output.position = camera.view_projection * float4(geometry.position, 1.0);
    output.normal = geometry.normal;
    output.color = float4(0.95, 0.28, 0.18, 1.0);
    return output;
}

fragment float4 capsule_fragment(VertexOutput input [[stage_in]],
                                 constant LightingUniforms &lighting [[buffer(0)]]) {
    return apply_lighting(input, lighting);
}
