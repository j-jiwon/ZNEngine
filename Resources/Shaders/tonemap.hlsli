// Final pass of the HDR pipeline: adds the bloom mip chain result onto the lit HDR
// SceneColor target, applies ACES filmic tone mapping, then gamma-encodes for
// display on the LDR back buffer.

cbuffer cbToneMapping : register(b0)
{
    float bloomIntensity;
    float3 _pad;
};

Texture2D sceneColorTexture : register(t0);
Texture2D bloomTexture : register(t1);
SamplerState sampler0 : register(s0);

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
    float2 uv : TEXCOORD;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output;
    output.pos = float4(input.pos, 1.0f);
    output.uv = input.uv;
    return output;
}

// Narkowicz 2015 ACES filmic fit
float3 ACESFilm(float3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;

    return saturate(
        (x*(a*x+b))
        /
        (x*(c*x+d)+e)
    );
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    float3 hdr = sceneColorTexture.Sample(sampler0, input.uv).rgb;
    float3 bloom = bloomTexture.Sample(sampler0, input.uv).rgb;
    hdr += bloom * bloomIntensity;

    hdr = ACESFilm(hdr);
    hdr = pow(hdr, 1.0/2.2);

    return float4(hdr, 1.0f);
}
