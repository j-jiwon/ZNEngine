
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
    float3 worldPos = worldPosTexture.Sample(sampler0, input.uv).rgb;

    // DIRECTIONAL LIGHT
    float3 L = normalize(-dirLightDirection);
    float3 V = normalize(viewPosition - worldPos);
    float3 H = normalize(L + V);
    
    float NdotL = max(dot(normal, L), 0.0f);
    float NdotH = max(dot(normal, H), 0.0f);
    
    float3 ambient = baseColor.rgb * dirAmbientIntensity;
    float3 diffuse = baseColor.rgb * dirLightColor * dirLightIntensity * NdotL;
    float3 specular = dirLightColor * dirLightIntensity * pow(NdotH, 32.0f) * 0.5f;
    
    float3 finalColor = ambient + diffuse + specular;
   
    // SPOT LIGHT
    float3 spotLightVec = spotLightPosition - worldPos; // Vector from fragment to light
    float spotDistance = length(spotLightVec);
    float3 spotDir = normalize(spotLightVec);

    // Distance attenuation
    float attenuation = 1.0f / (spotAttenuationConstant +
                                 spotAttenuationLinear * spotDistance +
                                 spotAttenuationQuadratic * spotDistance * spotDistance);

    // Cone cutoff (inner/outer)
    float theta = dot(spotDir, normalize(-spotLightDirection)); // Angle between light direction and fragment direction
    float epsilon = spotInnerCutoff - spotOuterCutoff; // Smooth transition
    float intensity = clamp((theta - spotOuterCutoff) / epsilon, 0.0f, 1.0f);

    // Diffuse calculation
    float spotNdotL = max(dot(normal, spotDir), 0.0f);
    float3 spotH = normalize(spotDir + V);
    float spotNdotH = max(dot(normal, spotH), 0.0f);
    float3 spotDiffuse = baseColor.rgb * spotLightColor * spotLightIntensity * spotNdotL * attenuation * intensity;
    float3 spotSpecular = spotLightColor * spotLightIntensity * pow(spotNdotH, 32.0f) * 0.5f * attenuation * intensity;
    
    // Combine all lighting
    finalColor += spotDiffuse + spotSpecular;

    return float4(finalColor, baseColor.a);
}
