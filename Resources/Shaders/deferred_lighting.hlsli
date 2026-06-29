
#define MAX_SPOT_LIGHTS   8
#define MAX_POINT_LIGHTS  8

struct SpotLightData
{
    float3 position;
    float intensity;
    float3 direction;
    float innerCutoff;
    float3 color;
    float outerCutoff;
    float attenuationConstant;
    float attenuationLinear;
    float attenuationQuadratic;
    float padding;
};

struct PointLightData
{
    float3 position;
    float intensity;
    float3 color;
    float radius;
    float attenuationConstant;
    float attenuationLinear;
    float attenuationQuadratic;
    float padding;
};

cbuffer cbLight : register(b0)
{
    // Directional Light
    float3 dirLightDirection;
    float dirLightIntensity;
    float3 dirLightColor;
    float dirAmbientIntensity;

    // Camera
    float3 viewPosition;
    int numSpotLights;

    // Shadow mapping
    row_major float4x4 lightViewProj;
    float2 shadowMapSize;
    float shadowBias;
    float shadowPCFRadius;

    // View mode
    int unlitMode;  // 1 = skip lighting, output baseColor directly (wireframe/unlit)
    int numPointLights;
    int2 _cbPad;

    // Spot Lights array
    SpotLightData spotLights[MAX_SPOT_LIGHTS];

    // Point Lights array
    PointLightData pointLights[MAX_POINT_LIGHTS];
};

// G-Buffer textures
Texture2D baseColorTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D depthTexture : register(t2);
Texture2D worldPosTexture : register(t3);
Texture2D armTexture : register(t4);  // ARM: R=AO, G=Roughness, B=Metallic
Texture2D shadowMap : register(t5);   // Shadow map depth texture

SamplerState sampler0 : register(s0);
SamplerComparisonState shadowSampler : register(s1);  // Comparison sampler for PCF

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
// Shadow Mapping Functions
// ============================================================================

// Calculate shadow factor using PCF (Percentage-Closer Filtering)
float CalculateShadow(float3 worldPos, float3 normal, float3 lightDir)
{
    // Normal offset bias - move sample position along surface normal to reduce shadow acne
    float normalOffsetScale = 0.05f;
    float3 offsetWorldPos = worldPos + normal * normalOffsetScale;

    // Transform world position to light clip space
    float4 lightSpacePos = mul(float4(offsetWorldPos, 1.0f), lightViewProj);

    // Perspective divide
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // Transform from [-1,1] to [0,1] UV space
    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = projCoords.y * -0.5f + 0.5f;  // Flip Y for UV coordinates

    // Outside shadow map bounds - fully lit
    if (projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f ||
        projCoords.z < 0.0f || projCoords.z > 1.0f)
    {
        return 1.0f;
    }

    // Slope-scale bias to reduce shadow acne
    float bias = max(shadowBias * (1.0f - dot(normal, lightDir)), shadowBias * 0.1f);
    float currentDepth = projCoords.z - bias;

    // PCF 3x3 filtering
    float shadow = 0.0f;
    float2 texelSize = 1.0f / shadowMapSize;
    int pcfRange = (int)shadowPCFRadius;
    int sampleCount = 0;

    for (int x = -pcfRange; x <= pcfRange; ++x)
    {
        for (int y = -pcfRange; y <= pcfRange; ++y)
        {
            float2 sampleUV = projCoords.xy + float2(x, y) * texelSize;
            float pcfDepth = shadowMap.SampleLevel(sampler0, sampleUV, 0).r;
            shadow += (currentDepth < pcfDepth) ? 1.0f : 0.0f;
            sampleCount++;
        }
    }

    return shadow / float(sampleCount);
}

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

    // Unlit / wireframe mode: skip all lighting, output raw baseColor
    if (unlitMode != 0)
        return float4(baseColor.rgb, baseColor.a);
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

    // DIRECTIONAL LIGHT (PBR) with Shadow
    float3 L_dir = normalize(-dirLightDirection);
    float shadow = CalculateShadow(worldPos, N, L_dir);

    // DEBUG: Visualize shadow projection values
    // Comment out the debug lines you don't need
    float4 debugLightSpacePos = mul(float4(worldPos, 1.0f), lightViewProj);
    float3 debugProjCoords = debugLightSpacePos.xyz / debugLightSpacePos.w;
    debugProjCoords.x = debugProjCoords.x * 0.5f + 0.5f;
    debugProjCoords.y = debugProjCoords.y * -0.5f + 0.5f;

    // DEBUG 0: Show world position (sanity check)
    // Objects should show varying colors based on their world position
    // return float4(worldPos * 0.1f + 0.5f, 1.0f);

    // DEBUG 1: Show UV coordinates (should be 0-1 for visible objects)
    // Red = U (0-1 left to right), Green = V (0-1 top to bottom)
    // return float4(debugProjCoords.xy, 0.0f, 1.0f);

    // DEBUG 1.5: Show raw light space position (before perspective divide)
    // Also show W component in blue - should be 1.0 for orthographic
    // return float4(debugLightSpacePos.xy * 0.5f + 0.5f, debugLightSpacePos.w, 1.0f);

    // DEBUG MATRIX: Test if values are truly zero or just small
    // If correct: R=1 (0][0]>0.01), G=1 ([1][1]>0.01), B=1 ([3][3]>0.9) = WHITE
    // If zeros: R=0, G=0, B=1 = BLUE
    // return float4(
    //     lightViewProj[0][0] > 0.01f ? 1.0f : 0.0f,
    //     lightViewProj[1][1] > 0.01f ? 1.0f : 0.0f,
    //     lightViewProj[3][3] > 0.9f ? 1.0f : 0.0f,
    //     1.0f
    // );

    // DEBUG 1.5: Show light space XY before perspective divide (raw clip space)
    // return float4(debugLightSpacePos.xy * 0.1f + 0.5f, 0.0f, 1.0f);

    // DEBUG 2: Show depth in light space (should be 0-1)
    // return float4(debugProjCoords.z, debugProjCoords.z, debugProjCoords.z, 1.0f);

    // DEBUG 3: Show shadow map sampled depth
    // float2 debugUV = debugProjCoords.xy;
    // float shadowMapDepth = shadowMap.SampleLevel(sampler0, debugUV, 0).r;
    // return float4(shadowMapDepth, shadowMapDepth, shadowMapDepth, 1.0f);

    // DEBUG 4: Show shadow factor
    // return float4(shadow, shadow, shadow, 1.0f);

    float3 radiance_dir = dirLightColor * dirLightIntensity * shadow;
    Lo += CalculatePBR(N, V, L_dir, radiance_dir, albedo, metallic, roughness);

    // SPOT LIGHTS (PBR) - loop through all active spot lights
    for (int i = 0; i < numSpotLights; ++i)
    {
        SpotLightData spot = spotLights[i];

        float3 spotLightVec = spot.position - worldPos;
        float spotDistance = length(spotLightVec);
        float3 L_spot = normalize(spotLightVec);

        // Distance attenuation
        float attenuation = 1.0f / (spot.attenuationConstant +
                                     spot.attenuationLinear * spotDistance +
                                     spot.attenuationQuadratic * spotDistance * spotDistance);

        // Cone cutoff (inner/outer)
        float theta = dot(L_spot, normalize(-spot.direction));
        float epsilon = spot.innerCutoff - spot.outerCutoff;
        float spotIntensity = clamp((theta - spot.outerCutoff) / epsilon, 0.0f, 1.0f);

        float3 radiance_spot = spot.color * spot.intensity * attenuation * spotIntensity;
        Lo += CalculatePBR(N, V, L_spot, radiance_spot, albedo, metallic, roughness);
    }

    // POINT LIGHTS (PBR) - omnidirectional, distance-attenuated
    for (int j = 0; j < numPointLights; ++j)
    {
        PointLightData pt = pointLights[j];

        float3 ptVec     = pt.position - worldPos;
        float  ptDist    = length(ptVec);
        float3 L_pt      = normalize(ptVec);

        // Hard cutoff at radius (smooth rolloff in last 10%)
        float  falloff   = saturate(1.0f - ptDist / max(pt.radius, 0.001f));
        falloff          = falloff * falloff;

        // Inverse-square attenuation
        float  atten     = 1.0f / (pt.attenuationConstant +
                                    pt.attenuationLinear    * ptDist +
                                    pt.attenuationQuadratic * ptDist * ptDist);

        float3 radiance_pt = pt.color * pt.intensity * atten * falloff;
        Lo += CalculatePBR(N, V, L_pt, radiance_pt, albedo, metallic, roughness);
    }

    // Combine ambient and direct lighting
    float3 finalColor = ambient + Lo;
    return float4(finalColor, baseColor.a);
}
