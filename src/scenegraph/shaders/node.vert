#version 450

// Every scene graph draw: positions in node-local units, transformed by the node's slot in the node buffer into the
// render target's pixels, then into clip space.

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color; // RGBA8, premultiplied by the renderer
layout(location = 3) in uint in_slot;  // index into nodes[]

// xy: pixel at the target's top-left corner, zw: 2 / target size in pixels.
layout(set = 0, binding = 0) uniform View {
    vec4 params;
} view;

// Per-node data, 48 bytes: the 2x2 matrix (a, b, c, d), (tx, ty, opacity, unused) and the premultiplied colour.
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
