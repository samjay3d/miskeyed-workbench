// Render Toy pass contract.
//
// The scene shader writes SceneSample. Workbench stores its color in
// the intermediate scene target, then exposes that target to the post-process shader.
struct SceneSample
{
    float4 color : SV_Target0;
};

// This binding is owned by Render Toy. QRhi supplies one combined image sampler at
// binding 1; Slang splits the same object into HLSL t1/s1 where that backend requires it.
[[vk::binding(1)]] Sampler2D sceneColor : register(t1) : register(s1);

float4 sampleScene(float2 uv)
{
    return sceneColor.Sample(uv);
}
