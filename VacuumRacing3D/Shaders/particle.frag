#version 330 core

in  vec2 vUV;
in  vec3 vColor;
in  float vAlpha;
out vec4 FragColor;

uniform sampler2D uSprite;
uniform vec3      uFogColor;
uniform float     uFogAmount;

void main() {
    vec4 tex = texture(uSprite, vUV);
    float a  = tex.a * vAlpha;
    if (a < 0.004) discard;
    vec3 rgb = mix(vColor * tex.rgb, uFogColor, uFogAmount);
    FragColor = vec4(rgb, a);
}
