#version 330 core

// Radial speed blur -> FXAA -> filmic tonemap -> vignette.

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScene;
uniform vec2      uTexelSize;
uniform float     uBlurStrength;   // 0 .. 1, driven by vehicle speed
uniform float     uVignette;
uniform float     uExposure;
uniform float     uSaturation;
uniform float     uFlash;          // white flash on collision / finish
uniform float     uEnableFXAA;

vec3 sampleScene(vec2 uv) { return texture(uScene, uv).rgb; }

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

vec3 fxaa(vec2 uv) {
    vec3  rgbM = sampleScene(uv);
    if (uEnableFXAA < 0.5) return rgbM;

    vec3 rgbNW = sampleScene(uv + vec2(-1.0, -1.0) * uTexelSize);
    vec3 rgbNE = sampleScene(uv + vec2( 1.0, -1.0) * uTexelSize);
    vec3 rgbSW = sampleScene(uv + vec2(-1.0,  1.0) * uTexelSize);
    vec3 rgbSE = sampleScene(uv + vec2( 1.0,  1.0) * uTexelSize);

    float lNW = luma(rgbNW), lNE = luma(rgbNE);
    float lSW = luma(rgbSW), lSE = luma(rgbSE), lM = luma(rgbM);

    float lMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));
    float lMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));
    if (lMax - lMin < max(0.0312, lMax * 0.125)) return rgbM;

    vec2 dir = vec2(-((lNW + lNE) - (lSW + lSE)), ((lNW + lSW) - (lNE + lSE)));
    float reduce = max((lNW + lNE + lSW + lSE) * 0.03125, 0.0078125);
    float rcpMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + reduce);
    dir = clamp(dir * rcpMin, vec2(-8.0), vec2(8.0)) * uTexelSize;

    vec3 rgbA = 0.5 * (sampleScene(uv + dir * (1.0 / 3.0 - 0.5)) +
                       sampleScene(uv + dir * (2.0 / 3.0 - 0.5)));
    vec3 rgbB = rgbA * 0.5 + 0.25 * (sampleScene(uv - dir * 0.5) + sampleScene(uv + dir * 0.5));
    float lB = luma(rgbB);
    return (lB < lMin || lB > lMax) ? rgbA : rgbB;
}

vec3 acesFilm(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 color = fxaa(vUV);

    // Radial blur away from the screen centre, scaled by speed.
    if (uBlurStrength > 0.001) {
        vec2  dir  = vUV - vec2(0.5);
        float mask = smoothstep(0.10, 0.62, length(dir));
        float amt  = uBlurStrength * mask * 0.055;
        vec3  acc  = color;
        for (int i = 1; i <= 6; ++i) {
            float t = float(i) / 6.0;
            acc += sampleScene(vUV - dir * amt * t);
        }
        color = acc / 7.0;
    }

    color *= uExposure;
    color  = acesFilm(color);

    float l = luma(color);
    color = mix(vec3(l), color, uSaturation);

    float d = length(vUV - vec2(0.5));
    color *= mix(1.0, smoothstep(0.92, 0.32, d), uVignette);

    color = mix(color, vec3(1.0), clamp(uFlash, 0.0, 1.0));

    // Gamma correction (rendering happens in linear space).
    color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));
    FragColor = vec4(color, 1.0);
}
