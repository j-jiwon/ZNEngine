// Bloom downsample step: 4-tap box filter that exploits bilinear sampling (each tap
// lands between 4 source texels), a cheap and standard way to halve resolution one
// mip level at a time without aliasing as badly as a single point/bilinear tap would.

Texture2D srcTexture : register(t0);
SamplerState clampSampler : register(s2);

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

float4 PS_Main(VS_OUT input) : SV_Target
{
    float w, h;
    srcTexture.GetDimensions(w, h);
    float2 texelSize = 1.0f / float2(w, h);

    float3 a = srcTexture.Sample(clampSampler, input.uv + texelSize * float2(-0.5f, -0.5f)).rgb;
    float3 b = srcTexture.Sample(clampSampler, input.uv + texelSize * float2( 0.5f, -0.5f)).rgb;
    float3 c = srcTexture.Sample(clampSampler, input.uv + texelSize * float2(-0.5f,  0.5f)).rgb;
    float3 d = srcTexture.Sample(clampSampler, input.uv + texelSize * float2( 0.5f,  0.5f)).rgb;

    return float4((a + b + c + d) * 0.25f, 1.0f);
}
