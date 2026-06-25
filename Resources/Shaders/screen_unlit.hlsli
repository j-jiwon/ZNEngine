// Unlit textured forward shader for render-target displays (TV screens, monitors).
// Samples tex_0 directly with no lighting calculation so the displayed content
// always appears at full brightness regardless of surface orientation.

cbuffer cbTransform : register(b0)
{
    row_major float4x4 gWorld;
    row_major float4x4 gView;
    row_major float4x4 gProjection;
};

cbuffer cbMaterial : register(b1)
{
    float4 albedoColor;
    float  metallic;
    float  roughness;
    float  ao;
    float  padding;
};

Texture2D    tex_0 : register(t0);
SamplerState sam_0 : register(s0);

struct VS_IN
{
    float3 pos    : POSITION;
    float4 color  : COLOR;
    float2 uv     : TEXCOORD;
    float3 normal : NORMAL;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;
    float4 worldPos = mul(float4(input.pos, 1.f), gWorld);
    float4 viewPos  = mul(worldPos, gView);
    output.pos = mul(viewPos, gProjection);
    output.uv  = input.uv;
    return output;
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    float4 texColor = tex_0.Sample(sam_0, input.uv);
    // If no texture bound, fall back to the material's albedo color.
    float lum = dot(texColor.rgb, float3(0.299f, 0.587f, 0.114f));
    if (lum < 0.005f && texColor.a < 0.005f)
        return albedoColor;
    return float4(texColor.rgb, 1.0f);
}
