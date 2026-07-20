#include <metal_stdlib>

using namespace metal;

struct VertexOutput {
    float4 position [[position]];
    float4 color;
};

vertex VertexOutput triangle_vertex(uint vertex_id [[vertex_id]]) {
    const float2 positions[] = {
        float2(0.0, 0.65),
        float2(-0.65, -0.55),
        float2(0.65, -0.55),
    };
    const float4 colors[] = {
        float4(1.0, 0.0, 0.0, 1.0),
        float4(0.0, 1.0, 0.0, 1.0),
        float4(0.0, 0.0, 1.0, 1.0),
    };

    VertexOutput output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.color = colors[vertex_id];
    return output;
}

fragment float4 triangle_fragment(VertexOutput input [[stage_in]]) { return input.color; }
