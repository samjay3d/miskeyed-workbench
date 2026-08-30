#include <miskeyed/workbench/slang_rhi/WorkbenchWindow.h>
#include <miskeyed/workbench/slang_rhi/ParameterInspector.h>
#include <miskeyed/workbench/slang_rhi/ShaderDocument.h>
#include <miskeyed/workbench/slang_rhi/SlangRhiWidget.h>
#include <miskeyed/workbench/slang_rhi/CodeEditor.h>
#include <miskeyed/workbench/slang_rhi/ShaderHighlighter.h>
#include <miskeyed/workbench/slang_rhi/LspClient.h>
#include <QAction>
#include <QClipboard>
#include <QComboBox>
#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

namespace miskeyed::workbench::slang_rhi {

namespace {

    // A good fixed-width coding font if available, falling back to the platform default.
    QFont monospaceFont(int pt = 11)
    {
        for (const QString& family :
            { QStringLiteral("Cascadia Code"), QStringLiteral("JetBrains Mono"),
                QStringLiteral("Consolas"), QStringLiteral("Menlo") }) {
            if (QFontDatabase::families().contains(family)) {
                QFont f(family, pt);
                f.setStyleHint(QFont::Monospace);
                return f;
            }
        }
        QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        f.setPointSize(pt);
        return f;
    }

    // Scene body for the scene pass: a few raymarched SDF primitives (sphere, box, torus)
    // on a checker ground with soft shadows and ambient occlusion, framed by an orbit
    // camera. Camera uniforms are additive offsets from a sensible base so the default
    // (all-zero) values already produce a nicely framed image. The scene pass renders this
    // into an offscreen texture (the G-buffer) that the post-process pass then samples.
    constexpr const char* kSceneBody = R"SLANG(
[UIGroup("Camera")] [UIName("Yaw")]      [UIWidget("angle")]  [UIRange(-3.14159, 3.14159)] [UIStep(0.01)] [UIUnits("rad")]
uniform float camYaw;
[UIGroup("Camera")] [UIName("Pitch")]    [UIWidget("angle")]  [UIRange(-1.5, 1.5)]         [UIStep(0.01)] [UIUnits("rad")]
uniform float camPitch;
[UIGroup("Camera")] [UIName("Distance")] [UIWidget("slider")] [UIRange(-4.0, 35.0)]        [UIStep(0.05)]
uniform float camDistance;
[UIGroup("Camera")] [UIName("FOV")]      [UIWidget("slider")] [UIRange(-40.0, 75.0)]       [UIStep(1.0)]  [UIUnits("deg")]
uniform float camFov;
[UIGroup("Camera")] [UIName("Pan X")]    [UIWidget("slider")] [UIRange(-10.0, 10.0)]       [UIStep(0.01)]
uniform float camPanX;
[UIGroup("Camera")] [UIName("Pan Y")]    [UIWidget("slider")] [UIRange(-10.0, 10.0)]       [UIStep(0.01)]
uniform float camPanY;

struct VSOut
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

[shader("vertex")]
VSOut vsMain(uint vertexID : SV_VertexID)
{
    float2 p = float2((vertexID << 1) & 2, vertexID & 2);
    VSOut o;
    o.uv = p;
    o.position = float4(p * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}

float sdSphere(float3 p, float r) { return length(p) - r; }
float sdBox(float3 p, float3 b) { float3 q = abs(p) - b; return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0); }
float sdTorus(float3 p, float2 t) { float2 q = float2(length(p.xz) - t.x, p.y); return length(q) - t.y; }

// Nearest surface as (distance, materialId): a few primitives on a ground plane.
float2 mapScene(float3 p)
{
    float2 res = float2(p.y + 1.0, 0.0);
    float2 sph = float2(sdSphere(p - float3(-1.35, -0.10, 0.0), 0.90), 1.0);
    if (sph.x < res.x) res = sph;
    float2 box = float2(sdBox(p - float3(1.25, -0.35, 0.25), float3(0.55, 0.65, 0.55)), 2.0);
    if (box.x < res.x) res = box;
    float2 tor = float2(sdTorus(p - float3(0.05, 0.05, -1.55), float2(0.55, 0.20)), 3.0);
    if (tor.x < res.x) res = tor;
    return res;
}

float3 sceneNormal(float3 p)
{
    float2 e = float2(0.0015, 0.0);
    return normalize(float3(
        mapScene(p + e.xyy).x - mapScene(p - e.xyy).x,
        mapScene(p + e.yxy).x - mapScene(p - e.yxy).x,
        mapScene(p + e.yyx).x - mapScene(p - e.yyx).x));
}

float softShadow(float3 ro, float3 rd, float mint, float maxt)
{
    float res = 1.0;
    float t = mint;
    for (int i = 0; i < 48; ++i)
    {
        float h = mapScene(ro + rd * t).x;
        res = min(res, 10.0 * h / t);
        t += clamp(h, 0.02, 0.30);
        if (h < 0.001 || t > maxt) break;
    }
    return saturate(res);
}

float calcAO(float3 p, float3 n)
{
    float occ = 0.0;
    float sca = 1.0;
    for (int i = 0; i < 5; ++i)
    {
        float hr = 0.01 + 0.12 * float(i) / 4.0;
        float d = mapScene(p + n * hr).x;
        occ += (hr - d) * sca;
        sca *= 0.95;
    }
    return saturate(1.0 - 3.0 * occ);
}

float3 materialColor(float id, float3 p)
{
    if (id < 0.5)
    {
        float checker = fmod(floor(p.x) + floor(p.z), 2.0);
        return lerp(float3(0.19, 0.20, 0.23), float3(0.30, 0.31, 0.35), checker);
    }
    if (id < 1.5) return float3(0.90, 0.42, 0.24); // sphere
    if (id < 2.5) return float3(0.26, 0.55, 0.74); // box
    return float3(0.64, 0.42, 0.80);               // torus
}

// Real viewport aspect from screen-space UV derivatives (no resolution uniform).
float viewAspect(float2 uv) { return abs(ddy(uv.y)) / max(abs(ddx(uv.x)), 1.0e-8); }

float3 renderScene(float2 uv, float aspect)
{
    float2 ndc = uv * 2.0 - 1.0;
    ndc.y = -ndc.y;
    ndc.x *= aspect;

    float yaw = camYaw + 0.7;
    float pitch = clamp(camPitch + 0.35, -1.45, 1.45);
    float dist = clamp(camDistance + 5.0, 1.6, 40.0);
    float fov = clamp(camFov + 45.0, 5.0, 120.0) * 0.017453292;

    float cy = cos(yaw), sy = sin(yaw);
    float cp = cos(pitch), sp = sin(pitch);
    float3 target = float3(camPanX, camPanY, 0.0);
    float3 ro = target + float3(sy * cp, sp, cy * cp) * dist;
    float3 fwd = normalize(target - ro);
    float3 right = normalize(cross(float3(0.0, 1.0, 0.0), fwd));
    float3 up = cross(fwd, right);
    float t = tan(fov * 0.5);
    float3 rd = normalize(fwd + right * ndc.x * t + up * ndc.y * t);

    float tt = 0.0;
    float id = -1.0;
    float3 pos = ro;
    for (int i = 0; i < 128; ++i)
    {
        pos = ro + rd * tt;
        float2 h = mapScene(pos);
        if (h.x < 0.001) { id = h.y; break; }
        tt += h.x;
        if (tt > 60.0) break;
    }

    float3 sky = lerp(float3(0.05, 0.07, 0.11), float3(0.13, 0.19, 0.30), saturate(rd.y * 0.5 + 0.5));
    if (id < 0.0) return sky;

    float3 n = sceneNormal(pos);
    float3 base = materialColor(id, pos);

    float3 lightDir = normalize(float3(0.6, 0.75, 0.35));
    float diff = saturate(dot(n, lightDir));
    float sh = softShadow(pos + n * 0.02, lightDir, 0.02, 20.0);
    float ao = calcAO(pos, n);

    float3 skyAmb = lerp(float3(0.10, 0.12, 0.17), float3(0.28, 0.33, 0.42), saturate(n.y * 0.5 + 0.5));
    float3 col = base * (skyAmb * ao + diff * sh * float3(1.0, 0.95, 0.85) * 1.6);

    float fres = pow(1.0 - saturate(dot(n, -rd)), 4.0);
    col += fres * 0.12 * ao;

    float fog = saturate(tt / 60.0);
    col = lerp(col, sky, fog * 0.30);
    return col;
}
)SLANG";

    QByteArray sceneShaderSource()
    {
        QByteArray s
            = "// slang-qt scene pass (camera-driven). Adjust the camera in the Camera panel.\n";
        s += kSceneBody;
        s += R"SLANG(
[shader("fragment")]
float4 psMain(VSOut input) : SV_Target0
{
    float3 col = renderScene(input.uv, viewAspect(input.uv));
    col = col / (col + 1.0);                       // Reinhard tone map
    col = pow(col, float3(0.4545, 0.4545, 0.4545)); // gamma
    return float4(col, 1.0);
}
)SLANG";
        return s;
    }

    QByteArray postShaderSource()
    {
        // A real post-process pass: it does NOT re-render the scene. The scene pass renders
        // into an offscreen color texture (the G-buffer); this shader samples that texture
        // and grades it. `uSceneColor`/`uSceneSampler` map to HLSL t1/s1 (SRB slot 1); the
        // globals constant buffer stays at b0, matching QRhi's D3D11 binding fallback.
        return R"SLANG(
// slang-qt post-process pass. Samples the scene G-buffer produced by the scene pass
// and grades it — this is the second stage that runs on top of the first.
// The vk::binding annotations only affect SPIR-V; the D3D11 path uses the t1/s1
// registers, matching the sampled-texture bound at SRB slot 1.
[[vk::binding(1)]] Texture2D uSceneColor : register(t1);
[[vk::binding(2)]] SamplerState uSceneSampler : register(s1);

[UIGroup("Grade")] [UIName("Tint")]     [UIRange(-1.0, 1.0)] [UIStep(0.01)]
uniform float3 tint;
[UIGroup("Grade")] [UIName("Exposure")] [UIWidget("slider")] [UIRange(-4.0, 4.0)] [UIStep(0.01)] [UIUnits("EV")]
uniform float exposure;
[UIGroup("Grade")] [UIName("Vignette")] [UIWidget("slider")] [UIRange(0.0, 2.0)]  [UIStep(0.01)]
uniform float vignette;

struct VSOut
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

[shader("vertex")]
VSOut vsMain(uint vertexID : SV_VertexID)
{
    float2 p = float2((vertexID << 1) & 2, vertexID & 2);
    VSOut o;
    o.uv = p;
    o.position = float4(p * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}

[shader("fragment")]
float4 psMain(VSOut input) : SV_Target0
{
    float3 c = uSceneColor.Sample(uSceneSampler, input.uv).rgb;
    c *= exp2(exposure);
    c *= (1.0 + tint);
    float2 q = input.uv * 2.0 - 1.0;
    float vig = 1.0 - saturate(dot(q, q) * vignette);
    return float4(c * vig, 1.0);
}
)SLANG";
    }

    // ---------------------------------------------------------------------------
    // Sample shader library. These are ready-to-run drop-ins the user can load from
    // the "Samples" toolbar menu to get a feel for what the workbench can do. Slang
    // (like every rasterizer) can't run the hardware ray-tracing pipeline through
    // QRhi, so the "ray tracing" flavour here is software raymarching in a fragment
    // shader — the same technique the built-in scene uses, extended to volumes.

    // Volume raymarch (scene pass): each pixel marches a ray through an animated fbm
    // density field, integrating extinction (Beer-Lambert) with a short sun-march for
    // single-scatter lighting. Shares the cam* uniforms so the Camera panel and the
    // Houdini mouse-nav fly the camera. Renders into the G-buffer; the post pass grades it.
    QByteArray volumeCloudsSample()
    {
        return R"SLANG(
// Sample: Volumetric cloud raymarch (camera-driven software ray tracing).
// Drag in either viewport to fly the camera (orbit / pan / zoom).
[UIGroup("Camera")] [UIName("Yaw")]      [UIWidget("angle")]  [UIRange(-3.14159, 3.14159)] [UIStep(0.01)] [UIUnits("rad")]
uniform float camYaw;
[UIGroup("Camera")] [UIName("Pitch")]    [UIWidget("angle")]  [UIRange(-1.5, 1.5)]         [UIStep(0.01)] [UIUnits("rad")]
uniform float camPitch;
[UIGroup("Camera")] [UIName("Distance")] [UIWidget("slider")] [UIRange(-4.0, 34.0)]        [UIStep(0.05)]
uniform float camDistance;
[UIGroup("Camera")] [UIName("FOV")]      [UIWidget("slider")] [UIRange(-50.0, 65.0)]       [UIStep(1.0)]  [UIUnits("deg")]
uniform float camFov;
[UIGroup("Camera")] [UIName("Pan X")]    [UIWidget("slider")] [UIRange(-10.0, 10.0)]       [UIStep(0.01)]
uniform float camPanX;
[UIGroup("Camera")] [UIName("Pan Y")]    [UIWidget("slider")] [UIRange(-10.0, 10.0)]       [UIStep(0.01)]
uniform float camPanY;
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
    float shape = fbm(p * 0.35) - (0.55 + coverage * 0.30);
    float slab = saturate(1.0 - abs(p.y - 1.2) / 1.6);
    return saturate(shape) * slab * (1.0 + densityMul);
}

float3 sunDirection() { float a = sunYaw + 0.6; return normalize(float3(sin(a), 0.55, cos(a))); }
float viewAspect(float2 uv) { return abs(ddy(uv.y)) / max(abs(ddx(uv.x)), 1.0e-8); }

[shader("fragment")]
float4 psMain(VSOut input) : SV_Target0
{
    float2 uv = input.uv;
    float aspect = viewAspect(uv);
    float2 ndc = uv * 2.0 - 1.0;
    ndc.y = -ndc.y; ndc.x *= aspect;

    float yaw = camYaw + 0.7;
    float pitch = clamp(camPitch + 0.15, -1.45, 1.45);
    float dist = clamp(camDistance + 6.0, 1.6, 40.0);
    float fov = clamp(camFov + 55.0, 5.0, 120.0) * 0.017453292;

    float cy = cos(yaw), sy = sin(yaw);
    float cp = cos(pitch), sp = sin(pitch);
    float3 target = float3(camPanX, 1.2 + camPanY, 0.0);
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
    return float4(col, 1.0);
}
)SLANG";
    }

    // Post: threshold bloom + radial chromatic aberration + ACES tonemap. Samples the
    // scene G-buffer at t1/s1 (SRB slot 1) exactly like the built-in post pass.
    QByteArray bloomSample()
    {
        return R"SLANG(
// Sample: Bloom + chromatic aberration post-process. Grades the scene G-buffer.
[[vk::binding(1)]] Texture2D uSceneColor : register(t1);
[[vk::binding(2)]] SamplerState uSceneSampler : register(s1);

[UIGroup("Bloom")] [UIName("Exposure")]  [UIWidget("slider")] [UIRange(-4.0, 4.0)] [UIStep(0.01)] [UIUnits("EV")]
uniform float exposure;
[UIGroup("Bloom")] [UIName("Threshold")] [UIWidget("slider")] [UIRange(-0.6, 0.4)] [UIStep(0.01)]
uniform float bloomThreshold;
[UIGroup("Bloom")] [UIName("Strength")]  [UIWidget("slider")] [UIRange(0.0, 2.0)]  [UIStep(0.01)]
uniform float bloomStrength;
[UIGroup("Bloom")] [UIName("Aberration")][UIWidget("slider")] [UIRange(0.0, 1.0)]  [UIStep(0.01)]
uniform float aberration;
[UIGroup("Bloom")] [UIName("Vignette")]  [UIWidget("slider")] [UIRange(0.0, 1.5)]  [UIStep(0.01)]
uniform float vignette;

struct VSOut { float4 position : SV_Position; float2 uv : TEXCOORD0; };

[shader("vertex")]
VSOut vsMain(uint vertexID : SV_VertexID)
{
    float2 p = float2((vertexID << 1) & 2, vertexID & 2);
    VSOut o; o.uv = p;
    o.position = float4(p * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}

float3 sampleScene(float2 uv) { return uSceneColor.Sample(uSceneSampler, saturate(uv)).rgb; }
float luma(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)); }

float3 bloomTap(float2 uv)
{
    float thr = 0.6 + bloomThreshold;
    float3 c = sampleScene(uv);
    return c * saturate((luma(c) - thr) / max(1.0 - thr, 1.0e-3));
}

[shader("fragment")]
float4 psMain(VSOut input) : SV_Target0
{
    float2 uv = input.uv;
    float2 q = uv * 2.0 - 1.0;

    // radial chromatic aberration
    float2 dir = q * (0.004 + aberration * 0.02);
    float3 col;
    col.r = sampleScene(uv - dir).r;
    col.g = sampleScene(uv).g;
    col.b = sampleScene(uv + dir).b;

    // cheap ring-tap bloom on the bright parts
    const float r = 0.006;
    float3 bloom = float3(0,0,0);
    bloom += bloomTap(uv + float2( r, 0.0));
    bloom += bloomTap(uv + float2(-r, 0.0));
    bloom += bloomTap(uv + float2(0.0,  r));
    bloom += bloomTap(uv + float2(0.0, -r));
    bloom += bloomTap(uv + float2( r,  r) * 0.7);
    bloom += bloomTap(uv + float2(-r,  r) * 0.7);
    bloom += bloomTap(uv + float2( r, -r) * 0.7);
    bloom += bloomTap(uv + float2(-r, -r) * 0.7);
    bloom += bloomTap(uv + float2( 2.0 * r, 0.0));
    bloom += bloomTap(uv + float2(-2.0 * r, 0.0));
    bloom += bloomTap(uv + float2(0.0,  2.0 * r));
    bloom += bloomTap(uv + float2(0.0, -2.0 * r));
    bloom /= 12.0;

    col += bloom * (0.6 + bloomStrength * 2.0);
    col *= exp2(exposure);

    // ACES-ish filmic tonemap
    float3 x = col;
    col = saturate((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14));

    float vig = 1.0 - saturate(dot(q, q) * (0.25 + vignette));
    return float4(col * vig, 1.0);
}
)SLANG";
    }

    // Post: retro CRT — barrel distortion, scanlines, aperture-grille mask, vignette.
    QByteArray crtSample()
    {
        return R"SLANG(
// Sample: CRT / scanline post-process. Grades the scene G-buffer.
[[vk::binding(1)]] Texture2D uSceneColor : register(t1);
[[vk::binding(2)]] SamplerState uSceneSampler : register(s1);

[UIGroup("CRT")] [UIName("Curvature")]  [UIWidget("slider")] [UIRange(0.0, 1.0)]  [UIStep(0.01)]
uniform float curvature;
[UIGroup("CRT")] [UIName("Scanline")]   [UIWidget("slider")] [UIRange(0.0, 1.0)]  [UIStep(0.01)]
uniform float scanline;
[UIGroup("CRT")] [UIName("Mask")]       [UIWidget("slider")] [UIRange(0.0, 1.0)]  [UIStep(0.01)]
uniform float maskStrength;
[UIGroup("CRT")] [UIName("Brightness")] [UIWidget("slider")] [UIRange(-0.5, 1.0)] [UIStep(0.01)]
uniform float brightness;
[UIGroup("CRT")] [UIName("Aberration")] [UIWidget("slider")] [UIRange(0.0, 1.0)]  [UIStep(0.01)]
uniform float aberration;

struct VSOut { float4 position : SV_Position; float2 uv : TEXCOORD0; };

[shader("vertex")]
VSOut vsMain(uint vertexID : SV_VertexID)
{
    float2 p = float2((vertexID << 1) & 2, vertexID & 2);
    VSOut o; o.uv = p;
    o.position = float4(p * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}

float2 curve(float2 uv, float amt)
{
    uv = uv * 2.0 - 1.0;
    float2 off = uv.yx * uv.yx * amt;
    uv += uv * off;
    return uv * 0.5 + 0.5;
}

[shader("fragment")]
float4 psMain(VSOut input) : SV_Target0
{
    float amt = 0.12 + curvature * 0.30;
    float2 uv = curve(input.uv, amt);
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
        return float4(0.0, 0.0, 0.0, 1.0); // outside the tube

    float ab = 0.0015 + aberration * 0.010;
    float3 col;
    col.r = uSceneColor.Sample(uSceneSampler, uv + float2(ab, 0.0)).r;
    col.g = uSceneColor.Sample(uSceneSampler, uv).g;
    col.b = uSceneColor.Sample(uSceneSampler, uv - float2(ab, 0.0)).b;

    // scanlines
    float s = sin(uv.y * 620.0 * 3.14159265);
    col *= 1.0 - (0.35 + scanline * 0.5) * s * s;

    // aperture-grille mask over 3 horizontal subpixels
    float m = frac(uv.x * 480.0);
    float3 mask = (m < 0.333) ? float3(1.0, 0.55, 0.55)
               : (m < 0.666) ? float3(0.55, 1.0, 0.55)
                             : float3(0.55, 0.55, 1.0);
    col *= lerp(float3(1.0, 1.0, 1.0), mask, saturate(0.4 + maskStrength));

    col *= (1.1 + brightness);
    float2 q = input.uv * 2.0 - 1.0;
    col *= 1.0 - saturate(dot(q, q) * 0.25);
    return float4(saturate(col), 1.0);
}
)SLANG";
    }

    // One entry per sample; `target` picks the editor slot (0 = scene, 1 = post).
    struct SampleShader {
        const char* name;
        int target;
        QByteArray (*source)();
    };

    QVector<SampleShader> sampleShaders()
    {
        return {
            { "Volume — Raymarched Clouds (scene)", 0, &volumeCloudsSample },
            { "Post — Bloom + Chromatic Aberration", 1, &bloomSample },
            { "Post — CRT / Scanlines", 1, &crtSample },
        };
    }

} // namespace

WorkbenchWindow::WorkbenchWindow(QWidget* parent)
    : QMainWindow(parent)
{
    buildUi();
    connectUi();
    // Load built-in shaders so both viewports render immediately instead of blank.
    m_sceneDocument->setSource(QString::fromUtf8(sceneShaderSource()));
    m_document->setSource(QString::fromUtf8(postShaderSource()));
    setupLanguageServer();
    setEditorTarget(m_editorTarget->currentIndex());
    m_sceneDocument->compile();
    m_document->compile();
}
WorkbenchWindow::WorkbenchWindow(const QString& shaderPath, QWidget* parent)
    : WorkbenchWindow(parent)
{
    if (!shaderPath.isEmpty())
        openShader(shaderPath);
}

void WorkbenchWindow::buildUi()
{
    setWindowTitle(QStringLiteral("Workbench"));
    resize(1600, 950);
    m_sceneDocument = new ShaderDocument(this);
    m_document = new ShaderDocument(this);

    m_sceneViewport = new SlangRhiWidget(this);
    m_sceneViewport->setDocument(m_sceneDocument);
    m_viewport = new SlangRhiWidget(this);
    m_viewport->setDocument(m_document);
    // The post viewport runs a real two-pass pipeline: it renders the scene document into
    // an offscreen texture (G-buffer), then its own document grades that texture on top.
    m_viewport->setScenePass(m_sceneDocument);
    m_editor = new CodeEditor(this);
    m_editor->setTabStopDistance(32);
    m_editor->setFont(monospaceFont());
    new ShaderHighlighter(m_editor->document());

    auto* cameraInspector = new ParameterInspector(this);
    cameraInspector->setModel(m_sceneDocument->parameters());
    auto* postInspector = new ParameterInspector(this);
    postInspector->setModel(m_document->parameters());
    m_diagnostics = new QPlainTextEdit(this);
    m_diagnostics->setReadOnly(true);
    m_diagnostics->setFont(monospaceFont(10));
    m_diagnostics->setPlaceholderText(QStringLiteral("No diagnostics — shader compiled cleanly."));

    auto labelled = [this](const QString& title, QWidget* w) {
        auto* box = new QWidget(this);
        auto* v = new QVBoxLayout(box);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(0);
        auto* lbl = new QLabel(title, box);
        lbl->setObjectName(QStringLiteral("PanelHeader"));
        v->addWidget(lbl);
        v->addWidget(w, 1);
        return box;
    };

    auto* views = new QSplitter(Qt::Horizontal, this);
    views->addWidget(labelled(QStringLiteral("1 · Scene  —  renders into the G-buffer   (drag: "
                                             "orbit · middle: pan · right/wheel: zoom)"),
        m_sceneViewport));
    views->addWidget(labelled(
        QStringLiteral("2 · Post-Process  —  samples the scene texture and grades it on top"),
        m_viewport));
    views->setStretchFactor(0, 1);
    views->setStretchFactor(1, 1);
    m_sceneViewport->setToolTip(QStringLiteral(
        "Scene pass. Rendered into an offscreen color texture (the G-buffer) that the\n"
        "post-process pass reads. Drag to move the camera (Houdini nav):\n"
        "  Left button  — orbit / tumble\n"
        "  Middle button — pan\n"
        "  Right button / wheel — dolly / zoom\n"
        "Edit this shader by choosing \"Scene shader\" in the Editing selector below."));
    m_viewport->setToolTip(QStringLiteral(
        "Post-process pass. This does NOT re-render the scene — it samples the scene\n"
        "G-buffer texture (uSceneColor) and grades it (exposure / tint / vignette), then\n"
        "draws on top. Edit it via \"Post-process shader\" in the Editing selector below.\n"
        "Camera drag here still moves the shared scene camera."));

    auto* tabs = new QTabWidget(this);
    tabs->addTab(cameraInspector, QStringLiteral("Camera"));
    tabs->addTab(postInspector, QStringLiteral("Post-Process"));
    m_diagTabIndex = tabs->addTab(m_diagnostics, QStringLiteral("Diagnostics"));
    m_tabs = tabs;

    auto* upper = new QSplitter(Qt::Horizontal, this);
    upper->addWidget(views);
    upper->addWidget(tabs);
    upper->setStretchFactor(0, 4);
    upper->setStretchFactor(1, 1);

    auto* editorBox = new QWidget(this);
    auto* ev = new QVBoxLayout(editorBox);
    ev->setContentsMargins(0, 0, 0, 0);
    ev->setSpacing(2);
    auto* editorBar = new QWidget(editorBox);
    auto* eh = new QHBoxLayout(editorBar);
    eh->setContentsMargins(6, 2, 6, 2);
    eh->addWidget(new QLabel(QStringLiteral("Editing:"), editorBar));
    m_editorTarget = new QComboBox(editorBar);
    m_editorTarget->addItems(
        { QStringLiteral("Scene shader"), QStringLiteral("Post-process shader") });
    m_editorTarget->setCurrentIndex(1);
    m_editorTarget->setToolTip(QStringLiteral(
        "Which shader the code editor below edits. The Scene and Post-process passes are\n"
        "two independent Slang shaders; they share the camera via their cam* uniforms."));
    eh->addWidget(m_editorTarget);

    // Persistent, color-coded compile-status pill. Sits right next to the editor so
    // feedback is where the user is looking (Fitts's Law) and always visible (Doherty
    // Threshold / Visibility of System Status). Clicking it jumps to the first error,
    // or recompiles when the shader is already clean.
    m_compileStatus = new QPushButton(editorBar);
    m_compileStatus->setObjectName(QStringLiteral("CompileStatus"));
    m_compileStatus->setCursor(Qt::PointingHandCursor);
    m_compileStatus->setFocusPolicy(Qt::NoFocus);
    eh->addSpacing(6);
    eh->addWidget(m_compileStatus);

    auto* navHint = new QLabel(
        QStringLiteral("Camera: drag = orbit · middle = pan · right/wheel = zoom  (Houdini)"),
        editorBar);
    navHint->setObjectName(QStringLiteral("HintLabel"));
    eh->addWidget(navHint);
    eh->addStretch(1);

    // Left: the editable Slang source. Right: the compiled output for a chosen backend.
    auto* sourceSide = new QWidget(editorBox);
    auto* sv = new QVBoxLayout(sourceSide);
    sv->setContentsMargins(0, 0, 0, 0);
    sv->setSpacing(0);
    auto* sourceHeader = new QLabel(QStringLiteral("Slang source"), sourceSide);
    sourceHeader->setObjectName(QStringLiteral("PanelHeader"));
    sv->addWidget(sourceHeader);
    sv->addWidget(m_editor, 1);

    m_generatedView = new CodeEditor(editorBox);
    m_generatedView->setReadOnly(true);
    m_generatedView->setFont(monospaceFont());
    m_generatedView->setPlaceholderText(
        QStringLiteral("Compile the shader to see generated code."));
    new ShaderHighlighter(m_generatedView->document());

    auto* genSide = new QWidget(editorBox);
    auto* gv = new QVBoxLayout(genSide);
    gv->setContentsMargins(0, 0, 0, 0);
    gv->setSpacing(0);
    auto* genBar = new QWidget(genSide);
    auto* gh = new QHBoxLayout(genBar);
    gh->setContentsMargins(8, 3, 8, 3);
    auto* genTitle = new QLabel(QStringLiteral("Compiled output"), genBar);
    genTitle->setObjectName(QStringLiteral("PanelHeaderInline"));
    gh->addWidget(genTitle);
    m_generatedTarget = new QComboBox(genBar);
    m_generatedTarget->setToolTip(QStringLiteral(
        "Backend to disassemble the current shader to: HLSL, GLSL, SPIR-V or Metal.\n"
        "Slang cross-compiles your Slang source to each of these."));
    gh->addWidget(m_generatedTarget);
    gh->addStretch(1);
    auto* copyBtn = new QPushButton(QStringLiteral("Copy"), genBar);
    copyBtn->setToolTip(QStringLiteral("Copy the generated code to the clipboard."));
    gh->addWidget(copyBtn);
    genBar->setObjectName(QStringLiteral("PanelHeader"));
    gv->addWidget(genBar);
    gv->addWidget(m_generatedView, 1);
    connect(copyBtn, &QPushButton::clicked, this, [this] {
        if (m_generatedView)
            QGuiApplication::clipboard()->setText(m_generatedView->toPlainText());
        statusBar()->showMessage(QStringLiteral("Copied generated code"), 1200);
    });

    auto* editorSplit = new QSplitter(Qt::Horizontal, editorBox);
    editorSplit->addWidget(sourceSide);
    editorSplit->addWidget(genSide);
    editorSplit->setStretchFactor(0, 3);
    editorSplit->setStretchFactor(1, 2);
    ev->addWidget(editorBar);
    ev->addWidget(editorSplit, 1);

    auto* root = new QSplitter(Qt::Vertical, this);
    root->addWidget(upper);
    root->addWidget(editorBox);
    root->setStretchFactor(0, 3);
    root->setStretchFactor(1, 2);
    setCentralWidget(root);
    setStatusBar(new QStatusBar(this));

    auto* tb = addToolBar(QStringLiteral("Shader"));
    tb->setMovable(false);
    auto* open = tb->addAction(QStringLiteral("Open"));
    open->setShortcut(QKeySequence::Open);
    auto* save = tb->addAction(QStringLiteral("Save"));
    save->setShortcut(QKeySequence::Save);
    auto* compile = tb->addAction(QStringLiteral("Compile"));
    compile->setShortcut(QKeySequence(QStringLiteral("Ctrl+Return")));
    auto* live = tb->addAction(QStringLiteral("Live"));
    live->setCheckable(true);
    live->setChecked(true);
    tb->addSeparator();
    auto* exportOut = tb->addAction(QStringLiteral("Export output…"));
    exportOut->setToolTip(QStringLiteral(
        "Save the currently shown compiled output (HLSL/GLSL/SPIR-V/Metal) to a file."));

    tb->addSeparator();
    auto* samplesBtn = new QToolButton(tb);
    samplesBtn->setText(QStringLiteral("Samples"));
    samplesBtn->setPopupMode(QToolButton::InstantPopup);
    samplesBtn->setToolTip(QStringLiteral(
        "Load a ready-made sample: a camera-driven volume raymarch or a post-process effect."));
    auto* samplesMenu = new QMenu(samplesBtn);
    for (const SampleShader& s : sampleShaders()) {
        QAction* a = samplesMenu->addAction(QString::fromUtf8(s.name));
        const int target = s.target;
        auto* fn = s.source;
        connect(a, &QAction::triggered, this, [this, target, fn] { loadSample(target, fn()); });
    }
    samplesBtn->setMenu(samplesMenu);
    tb->addWidget(samplesBtn);

    connect(open, &QAction::triggered, this, [this] {
        const auto p = QFileDialog::getOpenFileName(
            this, QStringLiteral("Open Slang shader"), {}, QStringLiteral("Slang (*.slang)"));
        if (!p.isEmpty())
            openShader(p);
    });
    connect(save, &QAction::triggered, this, [this] {
        if (m_editorDoc) {
            m_editorDoc->setSource(m_editor->toPlainText());
            m_editorDoc->save();
        }
    });
    connect(compile, &QAction::triggered, this, [this] {
        if (m_editorDoc) {
            m_editorDoc->setSource(m_editor->toPlainText());
            m_editorDoc->compile();
        }
    });
    connect(live, &QAction::toggled, this, [this](bool on) {
        m_sceneDocument->setLive(on);
        m_document->setLive(on);
    });
    connect(exportOut, &QAction::triggered, this, [this] {
        if (!m_generatedView || m_generatedView->toPlainText().isEmpty()) {
            statusBar()->showMessage(QStringLiteral("Nothing to export — compile first"), 1800);
            return;
        }
        const QString target
            = m_generatedTarget ? m_generatedTarget->currentText() : QStringLiteral("output");
        static const QMap<QString, QString> ext
            = { { QStringLiteral("HLSL"), QStringLiteral("hlsl") },
                  { QStringLiteral("GLSL"), QStringLiteral("glsl") },
                  { QStringLiteral("SPIR-V"), QStringLiteral("spvasm") },
                  { QStringLiteral("Metal"), QStringLiteral("metal") } };
        const QString suggested
            = QStringLiteral("shader.%1").arg(ext.value(target, QStringLiteral("txt")));
        const QString p = QFileDialog::getSaveFileName(
            this, QStringLiteral("Export compiled %1").arg(target), suggested);
        if (p.isEmpty())
            return;
        QFile f(p);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(m_generatedView->toPlainText().toUtf8());
            statusBar()->showMessage(QStringLiteral("Exported %1 to %2").arg(target, p), 2400);
        }
    });

    applyTheme();
}

void WorkbenchWindow::connectUi()
{
    connect(m_editor, &QPlainTextEdit::textChanged, this, [this] {
        if (!m_editorDoc)
            return;
        if (m_editorDoc->live())
            m_editorDoc->setSource(m_editor->toPlainText());
        setCompileState(CompileState::Dirty);
    });

    connect(m_compileStatus, &QPushButton::clicked, this, [this] {
        if (m_editorErrors > 0 || (!m_lastCompileOk && m_compileState == CompileState::Error))
            jumpToFirstError();
        else if (m_editorDoc) {
            m_editorDoc->setSource(m_editor->toPlainText());
            m_editorDoc->compile();
        }
    });

    const auto hookDocument = [this](ShaderDocument* doc) {
        connect(doc, &ShaderDocument::sourceChanged, this, [this, doc] {
            if (doc == m_editorDoc && m_editor->toPlainText() != doc->source()) {
                m_editor->blockSignals(true);
                m_editor->setPlainText(doc->source());
                m_editor->blockSignals(false);
            }
        });
        connect(doc, &ShaderDocument::diagnosticsChanged, this, [this, doc] {
            if (doc == m_editorDoc)
                m_diagnostics->setPlainText(doc->diagnostics());
        });
        connect(doc, &ShaderDocument::compilingChanged, this, [this, doc] {
            if (doc == m_editorDoc && doc->compiling()) {
                setCompileState(CompileState::Compiling);
                m_compileStatus->repaint(); // paint before the (synchronous) compile blocks the UI
            }
        });
        connect(doc, &ShaderDocument::compiled, this, [this, doc] {
            if (doc != m_editorDoc)
                return;
            m_lastCompileOk = true;
            recountDiagnostics();
            setCompileState(m_editorWarnings > 0 ? CompileState::Warn : CompileState::Ok);
            reloadGeneratedTargets();
        });
        connect(doc, &ShaderDocument::compileFailed, this, [this, doc](const QString&) {
            if (doc != m_editorDoc)
                return;
            m_lastCompileOk = false;
            recountDiagnostics();
            setCompileState(CompileState::Error);
        });
    };
    hookDocument(m_sceneDocument);
    hookDocument(m_document);

    connect(m_sceneViewport, &SlangRhiWidget::gpuError, this,
        [this](const QString& e) { m_diagnostics->appendPlainText(e); });
    connect(m_viewport, &SlangRhiWidget::gpuError, this,
        [this](const QString& e) { m_diagnostics->appendPlainText(e); });

    connect(m_editorTarget, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        &WorkbenchWindow::setEditorTarget);
    connect(m_generatedTarget, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [this] { refreshGeneratedView(); });

    // Camera uniforms are shared: mirror `cam*` values between the two passes so the
    // Qt editor is the single source of truth for the camera regardless of which panel
    // is used to edit it.
    connect(m_sceneDocument->parameters(), &ShaderParameterModel::parameterChanged, this,
        [this](const QString& name, const QVariant& value, int, int) {
            mirrorParameter(m_document, name, value);
        });
    connect(m_document->parameters(), &ShaderParameterModel::parameterChanged, this,
        [this](const QString& name, const QVariant& value, int, int) {
            mirrorParameter(m_sceneDocument, name, value);
        });
}

void WorkbenchWindow::setEditorTarget(int index)
{
    m_editorDoc = (index == 0) ? m_sceneDocument : m_document;
    m_editor->blockSignals(true);
    m_editor->setPlainText(m_editorDoc->source());
    m_editor->blockSignals(false);
    m_diagnostics->setPlainText(m_editorDoc->diagnostics());
    if (m_lsp) {
        const QString uri = documentUri(m_editorDoc);
        m_editor->setLanguageClient(m_lsp, uri);
        m_editor->setDiagnostics(m_diagnosticsByUri.value(uri));
    }
    recountDiagnostics();
    setCompileState(m_lastCompileOk ? (m_editorWarnings > 0 ? CompileState::Warn : CompileState::Ok)
                                    : CompileState::Error);
    reloadGeneratedTargets();
}

void WorkbenchWindow::loadSample(int target, const QByteArray& source)
{
    // Switch the editor to the sample's slot (scene or post), then swap in its source.
    if (m_editorTarget && m_editorTarget->currentIndex() != target)
        m_editorTarget->setCurrentIndex(target); // triggers setEditorTarget
    ShaderDocument* doc = (target == 0) ? m_sceneDocument : m_document;
    doc->setSource(QString::fromUtf8(source));
    m_editor->blockSignals(true);
    m_editor->setPlainText(doc->source());
    m_editor->blockSignals(false);
    doc->compile();
    statusBar()->showMessage(QStringLiteral("Loaded sample shader"), 1600);
}

QString WorkbenchWindow::documentUri(ShaderDocument* doc) const
{
    return doc == m_sceneDocument ? QStringLiteral("file:///slang-qt/scene.slang")
                                  : QStringLiteral("file:///slang-qt/post.slang");
}

void WorkbenchWindow::recountDiagnostics()
{
    m_editorErrors = 0;
    m_editorWarnings = 0;
    if (!m_editorDoc)
        return;
    for (const LspDiagnostic& d : m_diagnosticsByUri.value(documentUri(m_editorDoc))) {
        if (d.severity == 1)
            ++m_editorErrors;
        else if (d.severity == 2)
            ++m_editorWarnings;
    }
}

void WorkbenchWindow::setCompileState(CompileState state)
{
    m_compileState = state;
    updateCompileStatus();
}

void WorkbenchWindow::updateCompileStatus()
{
    if (!m_compileStatus)
        return;

    QString text;
    QString state; // drives the [state=...] stylesheet selector
    QString tip;
    switch (m_compileState) {
    case CompileState::Compiling:
        text = QStringLiteral("Compiling…");
        state = QStringLiteral("compiling");
        tip = QStringLiteral("Compiling the shader…");
        break;
    case CompileState::Dirty:
        text = QStringLiteral("● Modified");
        state = QStringLiteral("dirty");
        tip = QStringLiteral("Unsaved edits — compiling shortly. Click to compile now.");
        break;
    default: {
        const int ms = m_editorDoc ? m_editorDoc->lastCompileMs() : -1;
        if (!m_lastCompileOk || m_editorErrors > 0) {
            const int n = qMax(1, m_editorErrors);
            text = QStringLiteral("✕  %1 error%2")
                       .arg(n)
                       .arg(n == 1 ? QString() : QStringLiteral("s"));
            if (m_editorWarnings > 0)
                text += QStringLiteral(", %1 warning%2")
                            .arg(m_editorWarnings)
                            .arg(m_editorWarnings == 1 ? QString() : QStringLiteral("s"));
            state = QStringLiteral("error");
            tip = QStringLiteral("Compile failed. Click to jump to the first error.");
        } else if (m_editorWarnings > 0) {
            text = QStringLiteral("✓  Compiled · %1 warning%2")
                       .arg(m_editorWarnings)
                       .arg(m_editorWarnings == 1 ? QString() : QStringLiteral("s"));
            state = QStringLiteral("warn");
            tip = QStringLiteral("Compiled with warnings. Click to jump to the first one.");
        } else {
            text = ms >= 0 ? QStringLiteral("✓  Compiled · %1 ms").arg(ms)
                           : QStringLiteral("✓  Compiled");
            state = QStringLiteral("ok");
            tip = QStringLiteral("Shader is up to date. Click to recompile (Ctrl+Enter).");
        }
        break;
    }
    }

    m_compileStatus->setText(text);
    m_compileStatus->setToolTip(tip);
    if (m_compileStatus->property("state").toString() != state) {
        m_compileStatus->setProperty("state", state);
        m_compileStatus->style()->unpolish(m_compileStatus);
        m_compileStatus->style()->polish(m_compileStatus);
    }

    if (m_tabs && m_diagTabIndex >= 0) {
        m_tabs->setTabText(m_diagTabIndex,
            m_editorErrors > 0 ? QStringLiteral("Diagnostics (%1)").arg(m_editorErrors)
                               : QStringLiteral("Diagnostics"));
    }
}

void WorkbenchWindow::jumpToFirstError()
{
    if (!m_editorDoc)
        return;
    const QList<LspDiagnostic> diags = m_diagnosticsByUri.value(documentUri(m_editorDoc));

    // Prefer the earliest error; if there are none, fall back to the earliest warning.
    auto earliest = [&diags](int severity) -> const LspDiagnostic* {
        const LspDiagnostic* best = nullptr;
        for (const LspDiagnostic& d : diags) {
            if (d.severity != severity)
                continue;
            if (!best || d.range.startLine < best->range.startLine
                || (d.range.startLine == best->range.startLine
                    && d.range.startChar < best->range.startChar))
                best = &d;
        }
        return best;
    };
    const LspDiagnostic* target = earliest(1);
    if (!target)
        target = earliest(2);

    if (m_tabs && m_diagTabIndex >= 0)
        m_tabs->setCurrentIndex(m_diagTabIndex);
    if (target)
        m_editor->goToPosition(target->range.startLine, target->range.startChar);
}

void WorkbenchWindow::setupLanguageServer()
{
    QString exe = qEnvironmentVariable("SLANGD_PATH");
    if (exe.isEmpty() || !QFileInfo::exists(exe))
        exe = QStandardPaths::findExecutable(QStringLiteral("slangd"));
    if (exe.isEmpty()) {
        QStringList candidates;
        const QString slangRoot = qEnvironmentVariable("SLANG_ROOT");
        if (!slangRoot.isEmpty())
            candidates << slangRoot + QStringLiteral("/bin/slangd.exe");
        candidates << QCoreApplication::applicationDirPath() + QStringLiteral("/slangd.exe");
        for (const QString& candidate : candidates) {
            if (QFileInfo::exists(candidate)) {
                exe = candidate;
                break;
            }
        }
    }
    if (exe.isEmpty()) {
        statusBar()->showMessage(
            QStringLiteral("Slang language server (slangd) not found — IDE features disabled"),
            4000);
        return;
    }

    m_lsp = new LspClient(this);
    connect(m_lsp, &LspClient::diagnosticsReceived, this,
        [this](const QString& uri, const QList<LspDiagnostic>& diagnostics) {
            m_diagnosticsByUri.insert(uri, diagnostics);
            if (m_editorDoc && documentUri(m_editorDoc) == uri) {
                m_editor->setDiagnostics(diagnostics);
                recountDiagnostics();
                updateCompileStatus();
            }
        });
    connect(m_lsp, &LspClient::ready, this,
        [this] { statusBar()->showMessage(QStringLiteral("Slang language server ready"), 2000); });
    m_lsp->start(exe);
    m_lsp->openDocument(documentUri(m_sceneDocument), m_sceneDocument->source());
    m_lsp->openDocument(documentUri(m_document), m_document->source());
}

void WorkbenchWindow::reloadGeneratedTargets()
{
    if (!m_generatedTarget || !m_editorDoc)
        return;
    const QString current = m_generatedTarget->currentText();
    const QStringList targets = m_editorDoc->generatedTargets();
    QSignalBlocker block(m_generatedTarget);
    m_generatedTarget->clear();
    m_generatedTarget->addItems(targets);
    const int idx = targets.indexOf(current);
    if (idx >= 0)
        m_generatedTarget->setCurrentIndex(idx);
    block.unblock();
    refreshGeneratedView();
}

void WorkbenchWindow::refreshGeneratedView()
{
    if (!m_generatedView)
        return;
    const QString target = m_generatedTarget ? m_generatedTarget->currentText() : QString();
    m_generatedView->setPlainText(m_editorDoc ? m_editorDoc->generatedCode(target) : QString());
}

void WorkbenchWindow::applyTheme()
{
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget { background: #16171b; color: #c8ccd4; }
        QToolTip { background: #24262e; color: #e6e6e6; border: 1px solid #3a3d47; padding: 4px; }
        QLabel#PanelHeader, QWidget#PanelHeader {
            background: #22242b; color: #e6e6e6; font-weight: 600;
            padding: 4px 10px; border-bottom: 1px solid #2f323b;
        }
        QLabel#PanelHeaderInline { color: #e6e6e6; font-weight: 600; padding-right: 6px; }
        QLabel#HintLabel { color: #7f8794; padding-left: 12px; }
        QPlainTextEdit {
            background: #1b1c22; color: #c8ccd4; border: none;
            selection-background-color: #33467c; selection-color: #ffffff;
        }
        QToolBar { background: #1d1f25; border: none; spacing: 4px; padding: 4px; }
        QToolBar QToolButton {
            color: #d5d9e0; padding: 5px 12px; border-radius: 6px; background: transparent;
        }
        QToolBar QToolButton:hover { background: #2c2f38; }
        QToolBar QToolButton:pressed, QToolBar QToolButton:checked { background: #3a5fbf; color: #ffffff; }
        QToolBar::separator { background: #2f323b; width: 1px; margin: 4px 6px; }
        QPushButton {
            background: #2c2f38; color: #d5d9e0; border: 1px solid #3a3d47;
            border-radius: 6px; padding: 3px 12px;
        }
        QPushButton:hover { background: #363a45; border-color: #4a4e5a; }
        QPushButton:pressed { background: #3a5fbf; color: #ffffff; }
        QPushButton#CompileStatus {
            font-weight: 600; padding: 3px 12px; border-radius: 10px; border: 1px solid transparent;
        }
        QPushButton#CompileStatus[state="ok"]        { background: #17321f; color: #9ece6a; border-color: #2c5335; }
        QPushButton#CompileStatus[state="ok"]:hover   { border-color: #9ece6a; }
        QPushButton#CompileStatus[state="warn"]      { background: #33301b; color: #e0af68; border-color: #5a4d2e; }
        QPushButton#CompileStatus[state="warn"]:hover { border-color: #e0af68; }
        QPushButton#CompileStatus[state="error"]     { background: #35191f; color: #f7768e; border-color: #5a2b36; }
        QPushButton#CompileStatus[state="error"]:hover{ border-color: #f7768e; }
        QPushButton#CompileStatus[state="compiling"] { background: #1a2740; color: #7aa2f7; border-color: #2e4370; }
        QPushButton#CompileStatus[state="dirty"]     { background: #2a2c36; color: #c0caf5; border-color: #3a3d47; }
        QPushButton#CompileStatus[state="dirty"]:hover{ border-color: #7aa2f7; }
        QComboBox {
            background: #24262e; color: #e6e6e6; border: 1px solid #3a3d47;
            border-radius: 6px; padding: 3px 26px 3px 10px; min-height: 20px;
        }
        QComboBox:hover { border-color: #4a4e5a; }
        QComboBox::drop-down { border: none; width: 22px; }
        QComboBox QAbstractItemView {
            background: #24262e; color: #e6e6e6; border: 1px solid #3a3d47;
            selection-background-color: #3a5fbf; selection-color: #ffffff; outline: none;
        }
        QTabWidget::pane { border: 1px solid #2f323b; background: #1b1c22; }
        QTabBar::tab {
            background: #1d1f25; color: #9aa0ac; padding: 6px 16px;
            border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px;
        }
        QTabBar::tab:selected { background: #22242b; color: #e6e6e6; border-bottom: 2px solid #7aa2f7; }
        QTabBar::tab:hover:!selected { color: #c8ccd4; }
        QSplitter::handle { background: #0f1013; }
        QSplitter::handle:horizontal { width: 3px; }
        QSplitter::handle:vertical { height: 3px; }
        QSplitter::handle:hover { background: #3a5fbf; }
        QStatusBar { background: #1d1f25; color: #9aa0ac; border-top: 1px solid #2f323b; }
        QScrollBar:vertical { background: #1b1c22; width: 12px; margin: 0; }
        QScrollBar::handle:vertical { background: #353842; min-height: 28px; border-radius: 6px; margin: 2px; }
        QScrollBar::handle:vertical:hover { background: #454956; }
        QScrollBar:horizontal { background: #1b1c22; height: 12px; margin: 0; }
        QScrollBar::handle:horizontal { background: #353842; min-width: 28px; border-radius: 6px; margin: 2px; }
        QScrollBar::handle:horizontal:hover { background: #454956; }
        QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
        QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
    )"));
}

void WorkbenchWindow::mirrorParameter(
    ShaderDocument* target, const QString& name, const QVariant& value)
{
    if (m_syncing || !target || !name.startsWith(QLatin1String("cam")))
        return;
    m_syncing = true;
    target->parameters()->setValue(name, value);
    m_syncing = false;
}

void WorkbenchWindow::openShader(const QString& path)
{
    const QFileInfo info(path);
    if (!info.exists())
        return;
    ShaderDocument* doc = m_editorDoc ? m_editorDoc : m_document;
    doc->setFileUrl(QUrl::fromLocalFile(info.absoluteFilePath()));
    if (doc->load()) {
        m_editor->setPlainText(doc->source());
        doc->compile();
    }
}

} // namespace miskeyed::workbench::slang_rhi
