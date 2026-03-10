
cbuffer cbTransform : register(b0)
{
    float4x4 gWorld;
    float4x4 gView;
    float4x4 gProjection;
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

    // MVP transformation
    float4 worldPos = mul(float4(input.pos, 1.f), gWorld);
    float4 viewPos = mul(worldPos, gView);
    output.pos = mul(viewPos, gProjection);

    output.color = input.color;
    output.uv = input.uv;

    return output;
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    float4 texColor = tex_0.Sample(sam_0, input.uv);

    // If texture is mostly black or transparent, use albedo color
    // Otherwise multiply texture with albedo
    float texBrightness = dot(texColor.rgb, float3(0.299, 0.587, 0.114));
    float4 finalColor;

    if (texBrightness < 0.01 && texColor.a < 0.01)
    {
        // No texture or black texture - use albedo color only
        finalColor = albedoColor;
    }
    else
    {
        // Valid texture - multiply with albedo
        finalColor = texColor * albedoColor;
    }

    return finalColor;
}