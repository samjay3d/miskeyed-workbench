import miskeyed.ui;
import miskeyed.time;
import miskeyed.render_toy;

// Sample: Bloom + chromatic aberration post-process. Grades the scene G-buffer.

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

float3 sampleSceneClamped(float2 uv) { return sampleScene(saturate(uv)).rgb; }
float luma(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)); }

float3 bloomTap(float2 uv)
{
    float thr = 0.6 + bloomThreshold;
    float3 c = sampleSceneClamped(uv);
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
    col.r = sampleSceneClamped(uv - dir).r;
    col.g = sampleSceneClamped(uv).g;
    col.b = sampleSceneClamped(uv + dir).b;

    // cheap ring-tap bloom on the bright parts
    float r = 0.006 * (1.0 + 0.2 * sin(workbenchTime.time * 2.0));
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
