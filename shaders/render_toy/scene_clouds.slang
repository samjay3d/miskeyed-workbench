import miskeyed.ui;
import miskeyed.time;
import miskeyed.viewport_camera;
import miskeyed.render_toy;

// Sample: Volumetric cloud raymarch (camera-driven software ray tracing).
// camera comes from the published Viewport camera header.
// Drag in either viewport to fly the camera (orbit / pan / zoom).
[UIGroup("Clouds")] [UIName("Coverage")] [UIWidget("slider")] [UIRange(-0.5, 0.6)] [UIStep(0.01)]
uniform float coverage;
[UIGroup("Clouds")] [UIName("Density")]  [UIWidget("slider")] [UIRange(0.0, 3.0)]  [UIStep(0.01)]
uniform float densityMul;
[UIGroup("Clouds")] [UIName("Sun Angle")][UIWidget("angle")]  [UIRange(-3.14159, 3.14159)] [UIStep(0.01)] [UIUnits("rad")]
uniform float sunYaw;
[UIGroup("Clouds")] [UIName("Sun Tint")] [UIRange(-1.0, 1.0)] [UIStep(0.01)]
uniform float3 sunTint;

struct VSOut { float4 position : SV_Position; float2 uv : TEXCOORD0; };

[shader("vertex")]
VSOut vsMain(uint vertexID : SV_VertexID)
{
    float2 p = float2((vertexID << 1) & 2, vertexID & 2);
    VSOut o; o.uv = p;
    o.position = float4(p * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}

float hash(float3 p)
{
    p = frac(p * 0.3183099 + 0.1);
    p *= 17.0;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float valueNoise(float3 x)
{
    float3 i = floor(x);
    float3 f = frac(x);
    f = f * f * (3.0 - 2.0 * f);
    float n000 = hash(i + float3(0,0,0));
    float n100 = hash(i + float3(1,0,0));
    float n010 = hash(i + float3(0,1,0));
    float n110 = hash(i + float3(1,1,0));
    float n001 = hash(i + float3(0,0,1));
    float n101 = hash(i + float3(1,0,1));
    float n011 = hash(i + float3(0,1,1));
    float n111 = hash(i + float3(1,1,1));
    float nx00 = lerp(n000, n100, f.x);
    float nx10 = lerp(n010, n110, f.x);
    float nx01 = lerp(n001, n101, f.x);
    float nx11 = lerp(n011, n111, f.x);
    float nxy0 = lerp(nx00, nx10, f.y);
    float nxy1 = lerp(nx01, nx11, f.y);
    return lerp(nxy0, nxy1, f.z);
}

float fbm(float3 p)
{
    float a = 0.5;
    float sum = 0.0;
    for (int i = 0; i < 5; ++i) { sum += a * valueNoise(p); p *= 2.02; a *= 0.5; }
    return sum;
}

// Cloud density in a horizontal slab. Positive = inside cloud.
float cloudDensity(float3 p)
{
    p.xz += float2(workbenchTime.time * 0.18, workbenchTime.time * 0.07);
    float shape = fbm(p * 0.35) - (0.55 + coverage * 0.30);
    float slab = saturate(1.0 - abs(p.y - 1.2) / 1.6);
    return saturate(shape) * slab * (1.0 + densityMul);
}

float3 sunDirection() { float a = sunYaw + 0.6; return normalize(float3(sin(a), 0.55, cos(a))); }
float viewAspect(float2 uv) { return abs(ddy(uv.y)) / max(abs(ddx(uv.x)), 1.0e-8); }

[shader("fragment")]
SceneSample psMain(VSOut input)
{
    float2 uv = input.uv;
    float aspect = viewAspect(uv);
    float2 ndc = uv * 2.0 - 1.0;
    ndc.y = -ndc.y; ndc.x *= aspect;

    float yaw = camera.camYaw + 0.7;
    float pitch = clamp(camera.camPitch + 0.15, -1.45, 1.45);
    float dist = clamp(camera.camDistance + 6.0, 1.6, 40.0);
    float fov = clamp(camera.camFov + 55.0, 5.0, 120.0) * 0.017453292;

    float cy = cos(yaw), sy = sin(yaw);
    float cp = cos(pitch), sp = sin(pitch);
    float3 target = float3(camera.camPanX, 1.2 + camera.camPanY, 0.0);
    float3 ro = target + float3(sy * cp, sp, cy * cp) * dist;
    float3 fwd = normalize(target - ro);
    float3 right = normalize(cross(float3(0,1,0), fwd));
    float3 up = cross(fwd, right);
    float t = tan(fov * 0.5);
    float3 rd = normalize(fwd + right * ndc.x * t + up * ndc.y * t);

    float3 sun = sunDirection();
    float3 sunCol = sunTint + float3(1.0, 0.92, 0.78);
    float3 skyTop = float3(0.24, 0.42, 0.74);
    float3 skyHorizon = float3(0.72, 0.78, 0.86);
    float3 sky = lerp(skyHorizon, skyTop, saturate(rd.y * 0.55 + 0.35));
    sky += sunCol * pow(saturate(dot(rd, sun)), 48.0) * 0.7; // sun glow

    float tHit = 0.6;
    float3 scatter = float3(0,0,0);
    float trans = 1.0;
    for (int i = 0; i < 64; ++i)
    {
        float3 p = ro + rd * tHit;
        if (p.y < -0.5 || p.y > 3.2 || tHit > 26.0) break;
        float d = cloudDensity(p);
        if (d > 0.001)
        {
            float shadow = 1.0;
            float ts = 0.0;
            for (int j = 0; j < 6; ++j) { ts += 0.35; shadow *= exp(-cloudDensity(p + sun * ts) * 1.4); }
            float3 lit = sunCol * (0.35 + 0.65 * shadow) + float3(0.10, 0.13, 0.18); // + sky ambient
            float dt = 0.28;
            float absorb = exp(-d * dt * 6.0);
            scatter += trans * (1.0 - absorb) * lit * d;
            trans *= absorb;
            if (trans < 0.02) break;
        }
        tHit += 0.28;
    }

    float3 col = sky * trans + scatter;
    col = col / (col + 1.0);                        // Reinhard tone map
    col = pow(col, float3(0.4545, 0.4545, 0.4545)); // gamma
    SceneSample output;
    output.color = float4(col, 1.0);
    return output;
}
