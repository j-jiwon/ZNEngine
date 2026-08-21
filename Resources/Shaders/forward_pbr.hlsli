// Forward PBR shader for offscreen camera passes (CCTV, etc.) -- per-object variant: the world
// matrix arrives in cbTransform, one draw per object. See forward_pbr_instanced.hlsli for the
// batched variant, and forward_pbr_common.hlsli for the shading both share.
#include "forward_pbr_common.hlsli"

cbuffer cbTransform : register(b0)
{
    row_major float4x4 gWorld;
    row_major float4x4 gView;
    row_major float4x4 gProjection;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;
    float4 wp          = mul(float4(input.pos, 1.f), gWorld);
    output.pos         = mul(mul(wp, gView), gProjection);
    output.worldPos    = wp.xyz;
    output.worldNormal = mul(float4(input.normal, 0.f), gWorld).xyz;
    output.uv          = input.uv;
    return output;
}
