#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;   // unused, keeps the shared vertex layout
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aColor;

uniform mat4 uViewProj;

out vec2 vUV;
out vec3 vColor;
out float vAlpha;

void main() {
    vUV    = aUV;
    vColor = aColor;
    vAlpha = aNormal.x;                 // alpha smuggled through the normal slot
    gl_Position = uViewProj * vec4(aPosition, 1.0);
}
