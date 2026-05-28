#version 450

layout(location = 0) in vec2 in_local_pos;
layout(location = 1) in vec2 in_bounds_size;
layout(location = 2) in vec4 in_fill_color;
layout(location = 3) in vec4 in_stroke_color;
layout(location = 4) in vec4 in_params0;
layout(location = 5) in vec4 in_params1;
layout(location = 6) flat in uint in_shape_type;
layout(location = 7) flat in uint in_flags;

layout(location = 0) out vec4 out_color;

const uint k_shape_box = 0u;
const uint k_shape_circle = 1u;
const uint k_shape_line = 2u;
const uint k_shape_sector = 3u;

const uint k_flag_dashed = 1u;

float sd_box(vec2 p, vec2 hs)
{
    vec2 d = abs(p) - hs;
    return length(max(d, vec2(0.0))) + min(max(d.x, d.y), 0.0);
}

float sd_box_rounded(vec2 p, vec2 hs, float r)
{
    return sd_box(p, hs - vec2(r)) - r;
}

float sd_box_beveled(vec2 p, vec2 hs, float b)
{
    float d_box = sd_box(p, hs);
    float d_diag = (abs(p.x) + abs(p.y) - (hs.x + hs.y - b)) * 0.7071067811865476;
    return max(d_box, d_diag);
}

float sd_circle(vec2 p, float r)
{
    return length(p) - r;
}

float sd_segment(vec2 p, vec2 a, vec2 b)
{
    vec2 ab = b - a;
    vec2 ap = p - a;
    float ab2 = dot(ab, ab);
    float t = (ab2 > 0.0) ? clamp(dot(ap, ab) / ab2, 0.0, 1.0) : 0.0;
    return length(ap - ab * t);
}

void main()
{
    vec2 center = in_bounds_size * 0.5;
    vec2 hs = in_bounds_size * 0.5;
    vec2 p = in_local_pos - center;

    bool is_line = (in_shape_type == k_shape_line);
    bool is_sector = (in_shape_type == k_shape_sector);
    float stroke_width = is_line ? 0.0 : (is_sector ? in_params1.x : in_params0.z);

    float d;
    if (in_shape_type == k_shape_box)
    {
        float corner_r = in_params0.x;
        float bevel = in_params0.y;
        if (bevel > 0.0)
        {
            d = sd_box_beveled(p, hs, bevel);
        }
        else if (corner_r > 0.0)
        {
            d = sd_box_rounded(p, hs, corner_r);
        }
        else
        {
            d = sd_box(p, hs);
        }
    }
    else if (in_shape_type == k_shape_circle)
    {
        float r = min(hs.x, hs.y);
        d = sd_circle(p, r);
    }
    else if (in_shape_type == k_shape_line)
    {
        vec2 a = in_params0.xy;
        vec2 b = in_params0.zw;
        float thickness = in_params1.x;
        d = sd_segment(in_local_pos, a, b) - thickness * 0.5;

        if ((in_flags & k_flag_dashed) != 0u)
        {
            vec2 ab = b - a;
            float ab2 = dot(ab, ab);
            if (ab2 > 0.0)
            {
                float t = clamp(dot(in_local_pos - a, ab) / ab2, 0.0, 1.0);
                float arc = t * sqrt(ab2) + in_params1.w;
                float period = in_params1.y + in_params1.z;
                if (period > 0.0)
                {
                    float m = mod(arc, period);
                    if (m > in_params1.y)
                    {
                        discard;
                    }
                }
            }
        }
    }
    else if (in_shape_type == k_shape_sector)
    {
        float r_in = in_params0.x;
        float r_out = in_params0.y;
        float a_start = in_params0.z;
        float a_end = in_params0.w;
        float a_mid = 0.5 * (a_start + a_end);
        float a_half = 0.5 * (a_end - a_start);

        float cs = cos(-a_mid);
        float sn = sin(-a_mid);
        vec2 p_rot = vec2(p.x * cs - p.y * sn, p.x * sn + p.y * cs);
        float angle_local = atan(p_rot.y, p_rot.x);
        if (a_half < 3.14159 && abs(angle_local) > a_half)
        {
            discard;
        }
        d = max(length(p) - r_out, r_in - length(p));
    }
    else
    {
        discard;
    }

    float aa = max(fwidth(d), 1e-5);
    float outer_alpha = smoothstep(aa, -aa, d);
    if (outer_alpha <= 0.0)
    {
        discard;
    }

    float fill_alpha = (stroke_width > 0.0)
        ? smoothstep(aa, -aa, d + stroke_width)
        : outer_alpha;

    vec4 base;
    if (is_line)
    {
        base = in_fill_color;
    }
    else if (stroke_width > 0.0)
    {
        base = in_stroke_color;
    }
    else
    {
        base = in_fill_color;
    }
    vec4 inner = in_fill_color;

    vec3 rgb = mix(base.rgb, inner.rgb, fill_alpha);
    float a = mix(base.a, inner.a, fill_alpha) * outer_alpha;
    out_color = vec4(rgb, a);
}
