// Render Toy pass contract.
//
// The scene shader writes SceneSample. Workbench stores its color in
// the intermediate scene target, then exposes that target to the post-process shader.
struct SceneSample
{
    float4 color : SV_Target0;
};

// These bindings are owned by Render Toy. User post-process shaders sample the scene
// through sceneColor rather than redeclaring the host's QRhi binding layout.
[[vk::binding(1)]] Texture2D sceneColor : register(t1);
[[vk::binding(2)]] SamplerState sceneSampler : register(s1);

float4 sampleScene(float2 uv)
{
    return sceneColor.Sample(sceneSampler, uv);
}
