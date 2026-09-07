import miskeyed.ui;
import miskeyed.time;
import miskeyed.render_toy;

// The base Render Toy scene is real raster geometry. Slang owns the six vertices and
// material evaluation; the host owns only the draw submission and GPU resource lifetime.
[UIGroup("Material")] [UIName("Base color")]
uniform float3 baseColor = float3(0.72, 0.34, 0.16);
[UIGroup("Material")] [UIName("Roughness")] [UIRange(0.0, 1.0)] [UIStep(0.01)]
uniform float roughness = 0.35;
[UIGroup("Material")] [UIName("Metallic")] [UIRange(0.0, 1.0)] [UIStep(0.01)]
uniform float metallic = 0.0;

struct VSOut
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float3 normal : TEXCOORD1;
};

[shader("vertex")]
VSOut vsMain(uint vertexID : SV_VertexID)
{
    static const float2 positions[6] = {
        float2(-0.72, -0.72), float2(0.72, -0.72), float2(0.72, 0.72),
        float2(-0.72, -0.72), float2(0.72, 0.72), float2(-0.72, 0.72)
    };
    static const float2 uvs[6] = {
        float2(0.0, 0.0), float2(1.0, 0.0), float2(1.0, 1.0),
        float2(0.0, 0.0), float2(1.0, 1.0), float2(0.0, 1.0)
    };
    VSOut output;
    float2 p = positions[vertexID];
    output.position = float4(p.x, p.y + p.x * 0.12, 0.0, 1.0);
    output.uv = uvs[vertexID];
    output.normal = normalize(float3(-0.12, 0.18, 1.0));
    return output;
}

[shader("fragment")]
SceneSample psMain(VSOut input)
{
    float2 cell = floor(input.uv * 10.0);
    float weave = lerp(0.82, 1.0, fmod(cell.x + cell.y, 2.0));
    float3 n = normalize(input.normal);
    float3 light = normalize(float3(-0.45, 0.7, 0.8));
    float diffuse = saturate(dot(n, light));
    float specularPower = lerp(96.0, 4.0, roughness);
    float specular = pow(saturate(dot(n, normalize(light + float3(0.0, 0.0, 1.0)))), specularPower);
    float3 dielectric = baseColor * weave * (0.18 + 0.82 * diffuse);
    float3 metal = baseColor * (0.08 + diffuse) + baseColor * specular;
    SceneSample output;
    output.color = float4(lerp(dielectric + specular * 0.04, metal, metallic), 1.0);
    return output;
}
