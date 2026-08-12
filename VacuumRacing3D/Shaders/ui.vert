#version 330 core

layout(location = 0) in vec3 aPosition;  // x,y in pixels, z unused
layout(location = 1) in vec3 aNormal;    // x = alpha multiplier
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aColor;

uniform mat4 uProjection;

out vec2 vUV;
out vec3 vColor;
out float vAlpha;

void main() {
    vUV    = aUV;
    vColor = aColor;
    vAlpha = aNormal.x;
    gl_Position = uProjection * vec4(aPosition.xy, 0.0, 1.0);
}
