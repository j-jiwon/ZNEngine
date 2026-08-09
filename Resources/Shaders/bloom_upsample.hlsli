// Bloom upsample step: 3x3 tent filter upsamples the smaller mip; the pipeline's
// blend state (additive, see Shader::EnableAdditiveBlend) adds this straight onto
// the next-larger mip, which already holds that level's own bright-pass data.

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

    float3 sum = srcTexture.Sample(clampSampler, input.uv).rgb * 4.0f;
    sum += srcTexture.Sample(clampSampler, input.uv + texelSize * float2(-1,  0)).rgb * 2.0f;
    sum += srcTexture.Sample(clampSampler, input.uv + texelSize * float2( 1,  0)).rgb * 2.0f;
    sum += srcTexture.Sample(clampSampler, input.uv + texelSize * float2( 0, -1)).rgb * 2.0f;
    sum += srcTexture.Sample(clampSampler, input.uv + texelSize * float2( 0,  1)).rgb * 2.0f;
    sum += srcTexture.Sample(clampSampler, input.uv + texelSize * float2(-1, -1)).rgb;
    sum += srcTexture.Sample(clampSampler, input.uv + texelSize * float2( 1, -1)).rgb;
    sum += srcTexture.Sample(clampSampler, input.uv + texelSize * float2(-1,  1)).rgb;
    sum += srcTexture.Sample(clampSampler, input.uv + texelSize * float2( 1,  1)).rgb;

    return float4(sum / 16.0f, 1.0f);
}
