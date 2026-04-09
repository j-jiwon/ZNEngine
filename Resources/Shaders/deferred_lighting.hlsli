
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

    // Inverse view-projection matrix for position reconstruction
    float4x4 invViewProj;
};

// G-Buffer textures
Texture2D baseColorTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D depthTexture : register(t2);
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

// Reconstruct world position from depth
float3 ReconstructWorldPosition(float2 uv, float depth)
{
    // Convert UV to NDC coordinates
    float4 ndc;
    ndc.x = uv.x * 2.0f - 1.0f;
    ndc.y = (1.0f - uv.y) * 2.0f - 1.0f; // Flip Y
    ndc.z = depth;
    ndc.w = 1.0f;

    // Transform to world space
    float4 worldPos = mul(ndc, invViewProj);
    worldPos /= worldPos.w; // Perspective divide

    return worldPos.xyz;
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

    // Reconstruct world position
    float3 worldPos = ReconstructWorldPosition(input.uv, depth);

    // DIRECTIONAL LIGHT
    float3 dirLightDir = normalize(-dirLightDirection);
    float dirNdotL = max(dot(normal, dirLightDir), 0.0f);
    float3 dirAmbient = baseColor.rgb * dirAmbientIntensity;
    float3 dirDiffuse = baseColor.rgb * dirLightColor * dirLightIntensity * dirNdotL;

    // SPOT LIGHT
    float3 spotLightVec = spotLightPosition - worldPos; // Vector from fragment to light
    float spotDistance = length(spotLightVec);
    float3 spotLightDir = normalize(spotLightVec);

    // Distance attenuation
    float attenuation = 1.0f / (spotAttenuationConstant +
                                 spotAttenuationLinear * spotDistance +
                                 spotAttenuationQuadratic * spotDistance * spotDistance);

    // Cone cutoff (inner/outer)
    float3 spotDirection = normalize(spotLightDirection);
    float theta = dot(spotLightDir, -spotDirection); // Angle between light direction and fragment direction
    float epsilon = spotInnerCutoff - spotOuterCutoff; // Smooth transition
    float intensity = clamp((theta - spotOuterCutoff) / epsilon, 0.0f, 1.0f);

    // Diffuse calculation
    float spotNdotL = max(dot(normal, spotLightDir), 0.0f);
    float3 spotDiffuse = baseColor.rgb * spotLightColor * spotLightIntensity * spotNdotL * attenuation * intensity;

    // Combine all lighting
    float3 finalColor = dirAmbient + dirDiffuse + spotDiffuse;

    return float4(finalColor, baseColor.a);
}
