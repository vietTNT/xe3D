#version 330 core

// -----------------------------------------------------------------------------
//  Cook-Torrance PBR with a single directional sun, an analytic sky used both as
//  ambient IBL and as the reflection probe, PCF shadows and height fog.
// -----------------------------------------------------------------------------

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec3 vColor;
in vec4 vLightSpacePos;

out vec4 FragColor;

uniform vec3  uCameraPos;
uniform vec3  uLightDir;        // direction the light travels (from sun to scene)
uniform vec3  uLightColor;
uniform vec3  uAmbientSky;
uniform vec3  uAmbientGround;

uniform vec3  uAlbedo;
uniform float uMetallic;
uniform float uRoughness;
uniform vec3  uEmissive;
uniform float uAlpha;
uniform float uReflectivity;
uniform float uClearCoat;
uniform float uUseTexture;
uniform float uUseVertexColor;
uniform float uReceiveShadow;

uniform sampler2D uAlbedoMap;
uniform sampler2D uShadowMap;
uniform float uShadowTexel;
uniform float uShadowStrength;

uniform vec3  uFogColor;
uniform float uFogDensity;

const float PI = 3.14159265359;

vec3 skyRadiance(vec3 dir) {
    vec3  sunDir  = -normalize(uLightDir);
    float up      = clamp(dir.y, -1.0, 1.0);
    vec3  zenith  = vec3(0.18, 0.36, 0.72);
    vec3  horizon = vec3(0.78, 0.79, 0.80);
    vec3  ground  = vec3(0.20, 0.20, 0.18);

    vec3 col = mix(horizon, zenith, pow(clamp(up, 0.0, 1.0), 0.55));
    col = mix(col, ground, smoothstep(0.0, -0.22, up));

    float sd = max(dot(dir, sunDir), 0.0);
    // Warm late-afternoon scattering around the sun.
    col += uLightColor * pow(sd, 6.0) * 0.28;
    col += uLightColor * pow(sd, 220.0) * 1.4;
    col = mix(col, uLightColor * 0.85, pow(sd, 3.0) * 0.12 * (1.0 - smoothstep(0.0, 0.35, up)));
    return col;
}

float distributionGGX(vec3 N, vec3 H, float rough) {
    float a  = rough * rough;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 1e-6);
}

float geometrySchlick(float NdotV, float rough) {
    float r = rough + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float rough) {
    return geometrySchlick(max(dot(N, V), 0.0), rough) *
           geometrySchlick(max(dot(N, L), 0.0), rough);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float sampleShadow(vec3 N, vec3 L) {
    if (uReceiveShadow < 0.5 || uShadowStrength <= 0.0) return 1.0;

    vec3 proj = vLightSpacePos.xyz / max(vLightSpacePos.w, 1e-5);
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0 || proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0) return 1.0;

    float ndl  = max(dot(N, L), 0.0);
    float bias = max(0.0022 * (1.0 - ndl), 0.0007);

    float shadow = 0.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float d = texture(uShadowMap, proj.xy + vec2(x, y) * uShadowTexel).r;
            shadow += (proj.z - bias > d) ? 0.0 : 1.0;
        }
    }
    shadow /= 9.0;
    return mix(1.0, shadow, uShadowStrength);
}

void main() {
    vec3 baseColor = uAlbedo;
    if (uUseVertexColor > 0.5) baseColor *= vColor;
    if (uUseTexture > 0.5) baseColor *= texture(uAlbedoMap, vUV).rgb;

    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    if (!gl_FrontFacing) N = -N;

    vec3  L = -normalize(uLightDir);
    vec3  H = normalize(V + L);
    float rough = clamp(uRoughness, 0.035, 1.0);
    float metal = clamp(uMetallic, 0.0, 1.0);

    vec3 F0 = mix(vec3(0.04), baseColor, metal);

    float NDF = distributionGGX(N, H, rough);
    float G   = geometrySmith(N, V, L, rough);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3  numerator   = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-4;
    vec3  specular    = numerator / denominator;

    vec3 kD = (vec3(1.0) - F) * (1.0 - metal);
    float NdotL = max(dot(N, L), 0.0);

    float shadow = sampleShadow(N, L);
    vec3  direct = (kD * baseColor / PI + specular) * uLightColor * NdotL * shadow;

    // --- ambient: hemispheric sky/ground term + analytic environment reflection
    float hemi = 0.5 + 0.5 * N.y;
    vec3  ambient = mix(uAmbientGround, uAmbientSky, hemi) * baseColor * (1.0 - metal * 0.65);

    vec3  R      = reflect(-V, N);
    vec3  envCol = skyRadiance(normalize(R));
    float fres   = pow(clamp(1.0 - max(dot(N, V), 0.0), 0.0, 1.0), 5.0);
    float specAmount = mix(0.03 + 0.55 * fres, 1.0, metal);
    // Rough surfaces reflect far less; without this everything looks chalky.
    vec3  envSpec = envCol * specAmount * uReflectivity * (1.0 - rough) * (1.0 - rough * 0.55);
    envSpec *= mix(0.55, 1.0, shadow);

    // Clear coat: a thin, very glossy layer on top of the car paint.
    if (uClearCoat > 0.0) {
        vec3  Hc  = normalize(V + L);
        float ccD = distributionGGX(N, Hc, 0.07);
        float ccF = 0.04 + 0.96 * pow(clamp(1.0 - max(dot(N, V), 0.0), 0.0, 1.0), 5.0);
        envSpec += skyRadiance(normalize(R)) * ccF * uClearCoat * 0.20;
        direct   += uLightColor * ccD * ccF * uClearCoat * NdotL * shadow * 0.30;
    }

    vec3 color = direct + ambient + envSpec + uEmissive;

    // --- distance + height fog keeps the horizon soft
    float dist   = length(uCameraPos - vWorldPos);
    float height = clamp(1.0 - (vWorldPos.y - 2.0) * 0.012, 0.35, 1.0);
    float fog    = 1.0 - exp(-pow(dist * uFogDensity, 2.0) * height);
    color = mix(color, uFogColor, clamp(fog, 0.0, 0.92));

    FragColor = vec4(color, uAlpha);
}
