// Instanced variant of gbuffer.hlsli: world matrix comes from a per-instance StructuredBuffer
// (indexed by SV_InstanceID) instead of cbTransform, so many objects sharing the same Mesh can be
// drawn with a single DrawIndexedInstanced call. PS_Main is identical to gbuffer.hlsli.
//
// pack_matrix(row_major) only affects cbuffer members (gView/gProjection below) — legacy FXC
// (D3DCompileFromFile) always reads StructuredBuffer<float4x4> elements as column-major and
// ignores the pragma there, and `StructuredBuffer<row_major float4x4>` is a syntax error on this
// compiler. Since the CPU side (ZNMatrix4) writes row-major bytes, gInstanceWorlds ends up holding
// each matrix's TRANSPOSE from HLSL's point of view — compensated in VS_Main by swapping the
// mul() operand order for gWorld only (mul(v,M) == mul(M^T,v), so mul(gWorld,v) undoes it).
#pragma pack_matrix(row_major)

cbuffer cbViewProj : register(b0)
{
    float4x4 gView;
    float4x4 gProjection;
};

cbuffer cbMaterial : register(b1)
{
    float4 albedoColor;
    float metallic;
    float roughness;
    float ao;
    float useAlbedoTexture; // 1.0 = sample t0; 0.0 = albedoColor only
    float4 emissiveColor;   // rgb = glTF emissiveFactor, scales tex_emissive; a unused
    float roughnessScale;   // see gbuffer.hlsli
    float metallicScale;
    float useARMTexture;
};

StructuredBuffer<float4x4> gInstanceWorlds : register(t3);

Texture2D tex_0 : register(t0); // Albedo (BaseColor)
Texture2D tex_1 : register(t1); // Normal map
Texture2D tex_arm : register(t2); // ARM
Texture2D tex_emissive : register(t4); // t3 is taken by gInstanceWorlds above

SamplerState sam_0 : register(s0);

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
    float3 worldPos : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct PS_MRT_OUTPUT
{
    float4 baseColor : SV_Target0;
    float4 normal : SV_Target1;
    float4 depth : SV_Target2;
    float4 worldPos : SV_Target3;
    float4 arm : SV_Target4;
    float4 emissive : SV_Target5;
};

VS_OUT VS_Main(VS_IN input, uint instanceID : SV_InstanceID)
{
    VS_OUT output = (VS_OUT) 0;

    float4x4 gWorld = gInstanceWorlds[instanceID];

    // mul(gWorld, v) instead of mul(v, gWorld) — see the file header comment on why.
    float4 worldPos = mul(gWorld, float4(input.pos, 1.f));
    float4 viewPos = mul(worldPos, gView);
    output.pos = mul(viewPos, gProjection);

    output.worldPos = worldPos.xyz;
    output.normal = normalize(mul((float3x3)gWorld, input.normal));

    output.color = input.color;
    output.uv = input.uv;

    return output;
}

PS_MRT_OUTPUT PS_Main(VS_OUT input)
{
    PS_MRT_OUTPUT output;

    float3 baseColor;
    if (useAlbedoTexture > 0.5)
    {
        float4 texColor = tex_0.Sample(sam_0, input.uv);
        baseColor = texColor.rgb * albedoColor.rgb;
    }
    else
    {
        baseColor = albedoColor.rgb;
    }

    output.baseColor = float4(baseColor, albedoColor.a);

    float3 normalSample = tex_1.Sample(sam_0, input.uv).rgb;
    float3 N;
    if (dot(normalSample, normalSample) < 0.01)
    {
        N = normalize(input.normal);
    }
    else
    {
        float3 vN = normalize(input.normal);
        float3 up = abs(vN.y) < 0.999 ? float3(0, 1, 0) : float3(1, 0, 0);
        float3 vT = normalize(cross(up, vN));
        float3 vB = cross(vN, vT);

        float3 tsNormal = normalSample * 2.0 - 1.0;
        N = normalize(tsNormal.x * vT + tsNormal.y * vB + tsNormal.z * vN);
    }
    output.normal = float4(N * 0.5 + 0.5, 1.0);

    float4 armSample = tex_arm.Sample(sam_0, input.uv);
    float outAO, outRoughness, outMetallic;
    if (useARMTexture < 0.5)
    {
        outAO = ao; outRoughness = roughness; outMetallic = metallic;
    }
    else
    {
        outAO        = armSample.r;
        outRoughness = armSample.g;
        outMetallic  = armSample.b;
    }

    // See gbuffer.hlsli — scales the winning source, and floors roughness so GGX can't hit 0/0.
    const float kMinRoughness = 0.045;
    output.arm = float4(outAO,
                        max(saturate(outRoughness * roughnessScale), kMinRoughness),
                        saturate(outMetallic  * metallicScale), 1.0);

    // See gbuffer.hlsli for why this uses rasterizer screen-space depth rather than a VS-computed
    // z/w varying.
    output.depth = float4(input.pos.z, 0.0, 0.0, 1.0);

    output.worldPos = float4(input.worldPos, 1.0f);

    output.emissive = float4(tex_emissive.Sample(sam_0, input.uv).rgb * emissiveColor.rgb, 1.0);

    return output;
}
