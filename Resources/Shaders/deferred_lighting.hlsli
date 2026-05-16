
cbuffer cbLight : register(b0)
{
    // Directional Light
    float3 dirLightDirection;
    float dirLightIntensity;
    float3 dirLightColor;
    float dirAmbientIntensity;

    // Spot Light
    float3 spotLightPosition;
    float spotLightIntensity;
    float3 spotLightDirection;
    float spotInnerCutoff;
    float3 spotLightColor;
    float spotOuterCutoff;

    float3 viewPosition;
    float padding;

    // Spot light attenuation (constant, linear, quadratic)
    float spotAttenuationConstant;
    float spotAttenuationLinear;
    float spotAttenuationQuadratic;
    float padding2;
};

// G-Buffer textures
Texture2D baseColorTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D depthTexture : register(t2);
Texture2D worldPosTexture : register(t3);
Texture2D armTexture : register(t4);  // ARM: R=AO, G=Roughness, B=Metallic

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

// ============================================================================
// PBR Functions (Cook-Torrance BRDF)
// ============================================================================

static const float PI = 3.14159265359f;

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return num / denom;
}

// Geometry Function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float num = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return num / denom;
}

// Geometry Function (Smith's method)
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Fresnel Function (Schlick approximation)
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

// Calculate PBR lighting for a single light
float3 CalculatePBR(float3 N, float3 V, float3 L, float3 radiance,
                    float3 albedo, float metallic, float roughness)
{
    float3 H = normalize(V + L);

    // Calculate F0 (surface reflection at zero incidence)
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

    // Specular component
    float3 numerator = NDF * G * F;
    float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001f;
    float3 specular = numerator / denominator;

    // Energy conservation
    float3 kS = F;
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    kD *= 1.0f - metallic; // Metallic surfaces have no diffuse

    // Final lighting
    float NdotL = max(dot(N, L), 0.0f);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// ============================================================================

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output;
    output.pos = float4(input.pos, 1.0f);
    output.uv = input.uv;
    return output;
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    // Sample G-Buffer
    float4 baseColor = baseColorTexture.Sample(sampler0, input.uv);
    float4 encodedNormal = normalTexture.Sample(sampler0, input.uv);
    float depth = depthTexture.Sample(sampler0, input.uv).r;

    // Decode normal from [0,1] to [-1,1]
    float3 normal = encodedNormal.rgb * 2.0f - 1.0f;
    normal = normalize(normal);

    // Sample ARM texture (AO, Roughness, Metallic) - already converted in gbuffer pass
    float4 arm = armTexture.Sample(sampler0, input.uv);
    float ao = arm.r;
    float roughness = arm.g;
    float metallic = arm.b;

    ao = max(ao, 0.1f);
    
    // Reconstruct world position
    float3 worldPos = worldPosTexture.Sample(sampler0, input.uv).rgb;

    // Common vectors
    float3 N = normal;
    float3 V = normalize(viewPosition - worldPos);
    float3 albedo = baseColor.rgb;

    // Ambient lighting (with AO)
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * ao * dirAmbientIntensity;
    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    // DIRECTIONAL LIGHT (PBR)
    float3 L_dir = normalize(-dirLightDirection);
    float3 radiance_dir = dirLightColor * dirLightIntensity;
    Lo += CalculatePBR(N, V, L_dir, radiance_dir, albedo, metallic, roughness);

    // SPOT LIGHT (PBR)
    float3 spotLightVec = spotLightPosition - worldPos;
    float spotDistance = length(spotLightVec);
    float3 L_spot = normalize(spotLightVec);

    // Distance attenuation
    float attenuation = 1.0f / (spotAttenuationConstant +
                                 spotAttenuationLinear * spotDistance +
                                 spotAttenuationQuadratic * spotDistance * spotDistance);

    // Cone cutoff (inner/outer)
    float theta = dot(L_spot, normalize(-spotLightDirection));
    float epsilon = spotInnerCutoff - spotOuterCutoff;
    float spotIntensity = clamp((theta - spotOuterCutoff) / epsilon, 0.0f, 1.0f);

    float3 radiance_spot = spotLightColor * spotLightIntensity * attenuation * spotIntensity;
    Lo += CalculatePBR(N, V, L_spot, radiance_spot, albedo, metallic, roughness);

    // Combine ambient and direct lighting
    float3 finalColor = ambient + Lo;
    return float4(finalColor, baseColor.a);
}
