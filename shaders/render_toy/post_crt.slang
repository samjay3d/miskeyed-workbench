// Sample: CRT / scanline post-process. Grades the Render Toy scene target.
// The sceneColor binding is declared by the published Render Toy pass header.

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
    col.r = sceneColor.Sample(sceneSampler, uv + float2(ab, 0.0)).r;
    col.g = sceneColor.Sample(sceneSampler, uv).g;
    col.b = sceneColor.Sample(sceneSampler, uv - float2(ab, 0.0)).b;

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
