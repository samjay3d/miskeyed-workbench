// slang-qrhi 0.2 native reflection demo.
// Add/remove global shader parameters and the Qt inspector rebuilds from Slang reflection.

uniform float3 tint;
uniform float exposure;
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
    float2 uv = input.uv;
    float2 q = uv * 2.0 - 1.0;
    float glow = exp(-2.8 * dot(q, q));
    // Visible base so the viewport shows an image even at default (zero) uniforms.
    float3 base = lerp(float3(0.04, 0.07, 0.14), float3(0.18, 0.45, 0.85), uv.y);
    float3 col = base + glow * (0.35 + tint);
    col *= (1.0 + exposure);
    float edge = 1.0 - saturate(dot(q, q) * vignette);
    return float4(col * edge, 1.0);
}
