#version 450

layout(set = 0, binding = 1) uniform texture2D u_texture;
layout(set = 0, binding = 2) uniform sampler u_sampler;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(sampler2D(u_texture, u_sampler), in_uv);
}
