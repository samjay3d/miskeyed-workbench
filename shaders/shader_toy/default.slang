import miskeyed.time;
import miskeyed.shader_toy;

struct VSOut { float4 position : SV_Position; float2 uv : TEXCOORD0; };

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
    float2 p = (input.uv * shaderToy.resolution * 2.0 - shaderToy.resolution)
        / max(shaderToy.resolution.y, 1.0);
    float pulse = 0.5 + 0.5 * sin(workbenchTime.time + length(p) * 10.0);
    return float4(0.12 + pulse * float3(0.15, 0.45, 0.85), 1.0);
}
