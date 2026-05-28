#version 450

layout(push_constant) uniform PushConstants
{
    mat4 view_projection;
} pc;

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_size;
layout(location = 2) in vec2 in_uv_pos;
layout(location = 3) in vec2 in_uv_size;
layout(location = 4) in vec4 in_color;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

const vec2 k_quad_corners[6] = vec2[6](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

void main()
{
    vec2 corner = k_quad_corners[gl_VertexIndex];
    vec2 world_position = in_pos + corner * in_size;
    gl_Position = pc.view_projection * vec4(world_position, 0.0, 1.0);
    out_uv = in_uv_pos + corner * in_uv_size;
    out_color = in_color;
}
