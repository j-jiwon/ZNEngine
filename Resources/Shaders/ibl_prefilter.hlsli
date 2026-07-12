// Specular prefilter: one draw per (cube face, mip level). Same per-face basis
// reconstruction as ibl_irradiance.hlsli, then GGX importance-samples the source
// environment cube around N=V=R (Karis 2013 "Real Shading in Unreal Engine 4" /
// LearnOpenGL split-sum prefilter step). `roughness` is precomputed on the CPU as
// mip / (mipCount - 1) and passed in per draw.

cbuffer cbFace : register(b0)
{
    float3 faceForward; float roughness;
    float3 faceRight;   float _pad1;
    float3 faceUp;      float _pad2;
};

TextureCube srcEnvCube : register(t0);
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

static const float PI = 3.14159265359f;

// Van der Corput radical inverse (base 2), for a low-discrepancy Hammersley sequence.
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float rough)
{
    float a = rough * rough;

    float phi = 2.0f * PI * Xi.x;
    float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    float3 up = (abs(N.z) < 0.999f) ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    float2 ndc = input.uv * 2.0f - 1.0f;
    ndc.y = -ndc.y;
    float3 N = normalize(faceForward + ndc.x * faceRight + ndc.y * faceUp);
    float3 V = N;

    static const uint SAMPLE_COUNT = 32u;
    float3 prefilteredColor = float3(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);
        float3 L = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = saturate(dot(N, L));
        if (NdotL > 0.0f)
        {
            prefilteredColor += srcEnvCube.Sample(sampler0, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor /= max(totalWeight, 0.0001f);
    return float4(prefilteredColor, 1.0f);
}
