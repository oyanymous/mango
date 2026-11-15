#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;
layout(location = 3) in uint in_slot;

layout(set = 0, binding = 0) uniform View {
    vec4 params;
} view;

struct NodeData {
    vec4 matrix;
    vec4 translation_opacity;
    vec4 color;
};

layout(std430, set = 0, binding = 1) readonly buffer Nodes {
    NodeData nodes[];
};

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

void main() {
    NodeData node = nodes[in_slot];
    vec2 p = vec2(node.matrix.x * in_position.x + node.matrix.z * in_position.y + node.translation_opacity.x,
                  node.matrix.y * in_position.x + node.matrix.w * in_position.y + node.translation_opacity.y);
    gl_Position = vec4((p - view.params.xy) * view.params.zw - 1.0, 0.0, 1.0);
    out_uv = in_uv;
    out_color = in_color * node.color * node.translation_opacity.z;
}
