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
