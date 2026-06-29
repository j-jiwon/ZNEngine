
#define MAX_POINT_LIGHTS 8

struct PointLightData
{
    float3 position;
    float  intensity;
    float3 color;
    float  radius;
    float  attenuationConstant;
    float  attenuationLinear;
    float  attenuationQuadratic;
    float  padding;
};

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

cbuffer cbPointLights : register(b3)
{
    int  numPointLights;
    int3 _plPad;
    PointLightData pointLights[MAX_POINT_LIGHTS];
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
    float4 pos      : SV_Position;
    float3 worldPos : TEXCOORD0;
    float3 worldNml : NORMAL;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;
    float4 wp        = mul(float4(input.pos, 1.f), gWorld);
    output.pos       = mul(mul(wp, gView), gProjection);
    output.worldPos  = wp.xyz;
    output.worldNml  = mul(float4(input.normal, 0.f), gWorld).xyz;
    return output;
}

static const float PI = 3.14159265f;

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.f - F0) * pow(clamp(1.f - cosTheta, 0.f, 1.f), 5.f);
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    float3 N       = normalize(input.worldNml);
    float3 albedo  = albedoColor.rgb;

    // Simple directional ambient baseline
    float3 Lo = albedo * 0.05f;

    // Point lights (PBR-style diffuse + simple specular)
    for (int i = 0; i < numPointLights; ++i)
    {
        PointLightData pt = pointLights[i];

        float3 ptVec  = pt.position - input.worldPos;
        float  ptDist = length(ptVec);
        float3 L      = normalize(ptVec);

        float falloff = saturate(1.f - ptDist / max(pt.radius, 0.001f));
        falloff       = falloff * falloff;

        float  atten  = 1.f / (pt.attenuationConstant +
                                pt.attenuationLinear    * ptDist +
                                pt.attenuationQuadratic * ptDist * ptDist);

        float NdotL   = max(dot(N, L), 0.f);
        float3 radiance = pt.color * pt.intensity * atten * falloff;

        // Diffuse contribution
        Lo += albedo / PI * radiance * NdotL;
    }

    return float4(Lo, albedoColor.a);
}
