// Shadow Depth Pass Shader
// Renders scene from light's perspective to generate shadow map

cbuffer cbShadowTransform : register(b0)
{
    row_major float4x4 gWorld;
    row_major float4x4 gLightViewProj;
};

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
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;

    // Transform to world space
    float4 worldPos = mul(float4(input.pos, 1.0f), gWorld);

    // Transform to light clip space
    output.pos = mul(worldPos, gLightViewProj);

    return output;
}

// No pixel shader output needed - depth is automatically written
void PS_Main(VS_OUT input)
{
    // Depth is written to depth buffer automatically
}
