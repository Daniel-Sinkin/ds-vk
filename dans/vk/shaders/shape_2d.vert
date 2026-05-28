#version 450

layout(push_constant) uniform PushConstants
{
    mat4 view_projection;
} pc;

layout(location = 0) in vec4 in_bounds;
layout(location = 1) in vec4 in_fill_color;
layout(location = 2) in vec4 in_stroke_color;
layout(location = 3) in vec4 in_params0;
layout(location = 4) in vec4 in_params1;
layout(location = 5) in uint in_shape_type;
layout(location = 6) in uint in_flags;

layout(location = 0) out vec2 out_local_pos;
layout(location = 1) out vec2 out_bounds_size;
layout(location = 2) out vec4 out_fill_color;
layout(location = 3) out vec4 out_stroke_color;
layout(location = 4) out vec4 out_params0;
layout(location = 5) out vec4 out_params1;
layout(location = 6) flat out uint out_shape_type;
layout(location = 7) flat out uint out_flags;

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
    vec2 world_position = in_bounds.xy + corner * in_bounds.zw;
    gl_Position = pc.view_projection * vec4(world_position, 0.0, 1.0);
    out_local_pos = corner * in_bounds.zw;
    out_bounds_size = in_bounds.zw;
    out_fill_color = in_fill_color;
    out_stroke_color = in_stroke_color;
    out_params0 = in_params0;
    out_params1 = in_params1;
    out_shape_type = in_shape_type;
    out_flags = in_flags;
}
