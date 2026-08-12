#version 330 core

// Fullscreen triangle generated from gl_VertexID - no vertex buffer required.
out vec2 vNDC;

void main() {
    vec2 pos = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2) * 2.0 - 1.0;
    vNDC = pos;
    gl_Position = vec4(pos, 1.0, 1.0);
}
