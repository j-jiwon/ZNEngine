// Instanced variant of shadow_depth.hlsli: the world matrix comes from a per-instance
// StructuredBuffer (indexed by SV_InstanceID) instead of cbShadowTransform, so every object
// sharing a Mesh casts its shadow in a single DrawIndexedInstanced call (see ZNScene::RenderShadow).
// Still no pixel shader — depth is written by the rasterizer.
//
// pack_matrix(row_major) only affects cbuffer members (gLightViewProj below) — legacy FXC always
// reads StructuredBuffer<float4x4> elements as column-major and ignores the pragma there, so
// gInstanceWorlds holds each matrix's TRANSPOSE from HLSL's point of view (the CPU side writes
// row-major ZNMatrix4 bytes). Compensated by swapping the mul() operand order for gWorld only:
// mul(M^T, v) == mul(v, M). Same reasoning as gbuffer_instanced.hlsli.
#pragma pack_matrix(row_major)

cbuffer cbShadowViewProj : register(b0)
{
    float4x4 gLightViewProj;
};

// t3 matches gbuffer_instanced.hlsli — nothing else is bound during the shadow pass.
StructuredBuffer<float4x4> gInstanceWorlds : register(t3);

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

VS_OUT VS_Main(VS_IN input, uint instanceID : SV_InstanceID)
{
    VS_OUT output = (VS_OUT)0;

    float4x4 gWorld = gInstanceWorlds[instanceID];

    // mul(gWorld, v) instead of mul(v, gWorld) — see the file header comment on why.
    float4 worldPos = mul(gWorld, float4(input.pos, 1.0f));
    output.pos = mul(worldPos, gLightViewProj);

    return output;
}

// No pixel shader output needed - depth is automatically written
void PS_Main(VS_OUT input)
{
}
