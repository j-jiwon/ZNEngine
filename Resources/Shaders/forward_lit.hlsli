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
    // Scene directional light: direction=(0.5,-1,0.3), intensity=6, color=(0.5,0.5,0.5), ambient=1
    float3 N = normalize(input.worldNormal);
    float3 L = normalize(float3(-0.5f, 1.0f, -0.3f));

    float NdotL = max(dot(N, L), 0.0f);

    // Match main scene: ambient = ambientIntensity(1.0) * lightColor(0.5,0.5,0.5)
    //                   diffuse = NdotL * intensity(6.0) * lightColor(0.5,0.5,0.5)
    float3 ambient = float3(0.5f, 0.5f, 0.5f);
    float3 diffuse = NdotL * float3(3.0f, 3.0f, 3.0f);

    float3 finalColor = saturate(albedoColor.rgb * (ambient + diffuse));
    return float4(finalColor, albedoColor.a);
}
