// Main-camera skybox resolve: fills background pixels (GBuffer depth still at its
// far-plane clear value, i.e. no scene geometry there) of the already-lit HDR SceneColor
// with the visible skybox. Pixels that do have geometry are left untouched (clip()),
// so this must run after DeferredLightingRenderPass has written SceneColor.
// See SkyboxRenderer::DrawResolve / Passes/SkyboxPass.h.

cbuffer cbSkybox : register(b0)
{
    float3 camForward; float tanHalfFovX;
    float3 camRight;   float tanHalfFovY;
    float3 camUp;      float _pad;
};

Texture2D depthTexture : register(t0);  // GBuf_DepthCopy
TextureCube skyboxCube : register(t1);
SamplerState sampler0 : register(s0);

struct VS_IN
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output;
    output.pos = float4(input.pos, 1.0f);
    output.uv = input.uv;
    return output;
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    float depth = depthTexture.Sample(sampler0, input.uv).r;
    if (depth < 0.9999f)
        clip(-1); // real geometry here; leave SceneColor untouched

    float2 ndc = input.uv * 2.0f - 1.0f;
    ndc.y = -ndc.y;
    float3 rayDir = normalize(camForward + ndc.x * tanHalfFovX * camRight + ndc.y * tanHalfFovY * camUp);
    return float4(skyboxCube.Sample(sampler0, rayDir).rgb, 1.0f);
}
