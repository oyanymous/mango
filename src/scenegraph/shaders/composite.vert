#version 450

// A triangle covering the whole target, from gl_VertexIndex alone; uv (0, 0) is the top-left corner.
layout(location = 0) out vec2 out_uv;

void main() {
    out_uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(out_uv * 2.0 - 1.0, 0.0, 1.0);
}
