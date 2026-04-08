
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
};

// G-Buffer textures
Texture2D baseColorTexture : register(t0);
Texture2D normalTexture : register(t1);
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

    // Decode normal from [0,1] to [-1,1]
    float3 normal = encodedNormal.rgb * 2.0f - 1.0f;
    normal = normalize(normal);

    // DIRECTIONAL LIGHT
    float3 dirLightDir = normalize(-dirLightDirection);
    float dirNdotL = max(dot(normal, dirLightDir), 0.0f);
    float3 dirAmbient = baseColor.rgb * dirAmbientIntensity;
    float3 dirDiffuse = baseColor.rgb * dirLightColor * dirLightIntensity * dirNdotL;

    // SPOT LIGHT
    // We need world position to calculate spot light, but we don't have it in deferred rendering
    // For now, we'll just add a simple contribution based on normal alignment
    // In a real implementation, you'd need to reconstruct world position from depth
    float3 spotLightDir = normalize(-spotLightDirection);
    float spotNdotL = max(dot(normal, spotLightDir), 0.0f);
    float3 spotDiffuse = baseColor.rgb * spotLightColor * spotLightIntensity * spotNdotL * 0.3f; // Scale down

    // Combine all lighting
    float3 finalColor = dirAmbient + dirDiffuse + spotDiffuse;

    return float4(finalColor, baseColor.a);
}
