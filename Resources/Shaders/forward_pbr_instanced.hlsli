// Instanced variant of forward_pbr.hlsli: the world matrix comes from a per-instance
// StructuredBuffer (indexed by SV_InstanceID) instead of cbTransform, so every object sharing a
// Mesh is drawn into an offscreen camera's RT with a single DrawIndexedInstanced call
// (see ZNScene::AddOffscreenCamera). Shading is identical -- forward_pbr_common.hlsli.
//
// The instance buffer sits at t5, not t3: this shader's shadow map already owns t3 and the IBL
// irradiance cube owns t4 (both bound by Mesh::RenderForwardInstanced). t5 is the last per-material
// SRV slot in the shared root signature and nothing else in the forward path uses it.
//
// Legacy FXC always reads StructuredBuffer<float4x4> elements as column-major, while the CPU side
// writes row-major ZNMatrix4 bytes, so gInstanceWorlds holds each matrix's TRANSPOSE from HLSL's
// point of view. Compensated by swapping the mul() operand order for gWorld only:
// mul(M^T, v) == mul(v, M). Same reasoning as gbuffer_instanced.hlsli.
#include "forward_pbr_common.hlsli"

cbuffer cbViewProj : register(b0)
{
    row_major float4x4 gView;
    row_major float4x4 gProjection;
};

StructuredBuffer<float4x4> gInstanceWorlds : register(t5);

VS_OUT VS_Main(VS_IN input, uint instanceID : SV_InstanceID)
{
    VS_OUT output = (VS_OUT)0;

    float4x4 gWorld = gInstanceWorlds[instanceID];

    // mul(gWorld, v) instead of mul(v, gWorld) -- see the file header comment on why.
    float4 wp          = mul(gWorld, float4(input.pos, 1.f));
    output.pos         = mul(mul(wp, gView), gProjection);
    output.worldPos    = wp.xyz;
    output.worldNormal = mul(gWorld, float4(input.normal, 0.f)).xyz;
    output.uv          = input.uv;
    return output;
}
