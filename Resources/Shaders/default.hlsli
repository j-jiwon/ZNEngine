
cbuffer cbWorld : register(b0)
{
    float4x4 gWorld;
};

cbuffer cbMaterial : register(b1)
{
    float4 albedoColor;
    float metallic;
    float roughness;
    float ao;
    float padding;
};

Texture2D tex_0 : register(t0);
SamplerState sam_0 : register(s0);

struct VS_IN
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0;

    output.pos = mul(float4(input.pos, 1.f), gWorld);
    output.color = input.color;
    output.uv = input.uv;

    return output;
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    float4 texColor = tex_0.Sample(sam_0, input.uv);

    // Apply material albedo color
    float4 finalColor = texColor * albedoColor;

    return finalColor;
}