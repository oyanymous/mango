#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;

layout(set = 0, binding = 0) uniform Transform {
    vec4 rotation;
};

layout(location = 0) out vec2 out_uv;

void main() {
    vec2 p = mat2(rotation.x, rotation.y, -rotation.y, rotation.x) * in_position;
    gl_Position = vec4(p.x * rotation.z, p.y, 0.0, 1.0);
    out_uv = in_uv;
}
