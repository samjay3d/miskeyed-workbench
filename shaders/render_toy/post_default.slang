// Render Toy post-process pass. Samples the scene G-buffer produced by the scene pass
// and grades it — this is the second stage that runs on top of the first.
// Render Toy supplies the scene target and sampler through its published pass header.

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
    float3 c = sampleScene(input.uv).rgb;
    c *= exp2(exposure);
    c *= (1.0 + tint);
    float2 q = input.uv * 2.0 - 1.0;
    float vig = 1.0 - saturate(dot(q, q) * vignette);
    return float4(c * vig, 1.0);
}
