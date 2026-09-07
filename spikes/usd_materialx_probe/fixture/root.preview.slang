import miskeyed.ui;
import miskeyed.shader_toy;

[UIName("Base color")] [UIGroup("USD look override")]
uniform float3 baseColor = float3(0.8, 0.8, 0.8);
[UIName("Roughness")] [UIGroup("USD look override")] [UIRange(0.0, 1.0)]
uniform float roughness = 0.35;

struct PreviewVertex { float4 position : SV_Position; float2 uv : TEXCOORD0; };

[shader("vertex")]
PreviewVertex previewVertex(uint vertexID : SV_VertexID)
{
    float2 corner = float2((vertexID << 1) & 2, vertexID & 2);
    PreviewVertex output;
    output.uv = corner;
    output.position = float4(corner * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return output;
}

[shader("fragment")]
float4 previewFragment(PreviewVertex input) : SV_Target0
{
    float2 tile = floor(input.uv * 12.0);
    float checker = fmod(tile.x + tile.y, 2.0);
    float lighting = lerp(1.0 - 0.55 * roughness, 1.0, checker * 0.12);
    return float4(baseColor * lighting, 1.0);
}
