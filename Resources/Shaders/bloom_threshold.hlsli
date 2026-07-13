// Bright-pass extraction: keeps only the portion of SceneColor above `threshold`,
// scaled down smoothly near the cutoff so there's no hard edge. First stage of the
// bloom mip chain (BloomChain::Render); output feeds the downsample chain.

cbuffer cbBloomThreshold : register(b0)
{
    float threshold;
    float3 _pad;
};

Texture2D sceneColorTexture : register(t0);
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
    float3 color = sceneColorTexture.Sample(sampler0, input.uv).rgb;
    float brightness = max(color.r, max(color.g, color.b));
    float contribution = max(brightness - threshold, 0.0f) / max(brightness, 0.0001f);
    return float4(color * contribution, 1.0f);
}
