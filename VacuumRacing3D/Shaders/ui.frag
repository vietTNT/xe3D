#version 330 core

in  vec2 vUV;
in  vec3 vColor;
in  float vAlpha;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform float     uMode;   // 0 = solid, 1 = rgba texture, 2 = red-channel glyph atlas

void main() {
    vec4 c = vec4(vColor, vAlpha);
    if (uMode > 1.5) {
        c.a *= texture(uTexture, vUV).r;
    } else if (uMode > 0.5) {
        vec4 t = texture(uTexture, vUV);
        c.rgb *= t.rgb;
        c.a   *= t.a;
    }
    if (c.a < 0.002) discard;
    FragColor = c;
}
