#version 330 core

in  vec2 vNDC;
out vec4 FragColor;

uniform mat4  uInvViewProj;
uniform vec3  uCameraPos;
uniform vec3  uLightDir;
uniform vec3  uLightColor;
uniform float uTime;

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 5; ++i) {
        v += a * valueNoise(p);
        p *= 2.03;
        a *= 0.5;
    }
    return v;
}

void main() {
    vec4 near = uInvViewProj * vec4(vNDC, -1.0, 1.0);
    vec4 far  = uInvViewProj * vec4(vNDC,  1.0, 1.0);
    vec3 dir  = normalize(far.xyz / far.w - near.xyz / near.w);

    vec3  sunDir  = -normalize(uLightDir);
    float up      = dir.y;

    vec3 zenith  = vec3(0.16, 0.34, 0.70);
    vec3 horizon = vec3(0.80, 0.80, 0.80);
    vec3 ground  = vec3(0.19, 0.20, 0.18);

    vec3 col = mix(horizon, zenith, pow(clamp(up, 0.0, 1.0), 0.55));
    col = mix(col, ground, smoothstep(0.0, -0.25, up));

    float sd = max(dot(dir, sunDir), 0.0);
    col += uLightColor * pow(sd, 6.0) * 0.30;                 // broad glow
    col += uLightColor * pow(sd, 1800.0) * 9.0;               // sun disk
    col = mix(col, uLightColor * 0.9, pow(sd, 3.0) * 0.16 * (1.0 - smoothstep(0.0, 0.4, up)));

    // ---- clouds: two layers of drifting fbm projected on a virtual dome
    if (up > 0.005) {
        vec2  uv     = dir.xz / max(up, 0.02);
        float drift  = uTime * 0.004;
        float lowF   = fbm(uv * 0.55 + vec2(drift, drift * 0.6));
        float highF  = fbm(uv * 1.7 - vec2(drift * 1.6, drift));
        float cover  = smoothstep(0.48, 0.86, lowF * 0.75 + highF * 0.35);
        float fade   = smoothstep(0.0, 0.30, up);

        float lit    = pow(clamp(dot(dir, sunDir) * 0.5 + 0.5, 0.0, 1.0), 3.0);
        vec3  bright = mix(vec3(0.96, 0.95, 0.93), uLightColor * 1.15, lit * 0.55);
        vec3  shade  = mix(vec3(0.55, 0.57, 0.62), vec3(0.72, 0.68, 0.66), lit);
        vec3  cloud  = mix(shade, bright, smoothstep(0.45, 0.95, lowF));

        col = mix(col, cloud, cover * fade * 0.92);
    }

    FragColor = vec4(col, 1.0);
}
