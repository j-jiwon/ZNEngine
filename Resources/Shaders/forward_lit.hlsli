cbuffer cbTransform : register(b0)
{
    row_major float4x4 gWorld;
    row_major float4x4 gView;
    row_major float4x4 gProjection;
};

cbuffer cbMaterial : register(b1)
{
    float4 albedoColor;
    float metallic;
    float roughness;
    float ao;
    float padding;
};

struct VS_IN
{
    float3 pos    : POSITION;
    float4 color  : COLOR;
    float2 uv     : TEXCOORD;
    float3 normal : NORMAL;
};

struct VS_OUT
{
    float4 pos         : SV_Position;
    float3 worldNormal : NORMAL;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;
    float4 worldPos    = mul(float4(input.pos, 1.f), gWorld);
    output.pos         = mul(mul(worldPos, gView), gProjection);
    output.worldNormal = mul(float4(input.normal, 0.f), gWorld).xyz;
    return output;
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    float3 N = normalize(input.worldNormal);
    float3 L = normalize(float3(-0.5f, 1.0f, -0.3f));

    float NdotL = max(dot(N, L), 0.0f);

    // ambient + diffuse stay in [0,1] (0.12 + 0.88 = 1.0) — no saturation/washout
    float3 finalColor = albedoColor.rgb * (0.12f + NdotL * 0.88f);
    return float4(finalColor, albedoColor.a);
}
