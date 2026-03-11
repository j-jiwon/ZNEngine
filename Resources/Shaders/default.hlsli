
cbuffer cbTransform : register(b0)
{
    float4x4 gWorld;
    float4x4 gView;
    float4x4 gProjection;
};

cbuffer cbMaterial : register(b1)
{
    float4 albedoColor;
    float metallic;
    float roughness;
    float ao;
    float padding;
};

cbuffer cbLight : register(b2)
{
    // Primary light (can be any type)
    float3 gLightPosition;
    int gLightType; // 0=Directional, 1=Point, 2=Spot

    float3 gLightDirection;
    float gLightIntensity;

    float3 gLightColor;
    float gAmbientIntensity;

    float3 gCameraPos;
    float gCutoffAngle; // Spot light inner cutoff (cosine)

    float gConstant;
    float gLinear;
    float gQuadratic;
    float gOuterCutoffAngle; // Spot light outer cutoff (cosine)

    // Secondary directional light (always active)
    float3 gDirLightDirection;
    float gDirLightIntensity;

    float3 gDirLightColor;
    float gDirLightPadding;
};

Texture2D tex_0 : register(t0);
SamplerState sam_0 : register(s0);

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
    float3 worldPos : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0;

    // MVP transformation
    float4 worldPos = mul(float4(input.pos, 1.f), gWorld);
    float4 viewPos = mul(worldPos, gView);
    output.pos = mul(viewPos, gProjection);

    // Pass world position for lighting
    output.worldPos = worldPos.xyz;

    // Transform normal to world space (assuming uniform scaling)
    output.normal = normalize(mul(input.normal, (float3x3)gWorld));

    output.color = input.color;
    output.uv = input.uv;

    return output;
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    // Sample texture
    float4 texColor = tex_0.Sample(sam_0, input.uv);

    // Determine base color
    float texBrightness = dot(texColor.rgb, float3(0.299, 0.587, 0.114));
    float3 baseColor;

    if (texBrightness < 0.01 && texColor.a < 0.01)
    {
        // No texture or black texture - use albedo color only
        baseColor = albedoColor.rgb;
    }
    else
    {
        // Valid texture - multiply with albedo
        baseColor = texColor.rgb * albedoColor.rgb;
    }

    // Normalize interpolated normal
    float3 N = normalize(input.normal);
    float3 V = normalize(gCameraPos - input.worldPos); // View direction

    // Ambient lighting
    float3 ambient = gAmbientIntensity * baseColor;

    // Calculate light direction and attenuation based on light type
    float3 L;
    float attenuation = 1.0;
    float spotEffect = 1.0;

    if (gLightType == 0) // Directional Light
    {
        L = normalize(-gLightDirection);
        attenuation = 1.0;
    }
    else if (gLightType == 1) // Point Light
    {
        float3 lightVec = gLightPosition - input.worldPos;
        float distance = length(lightVec);
        L = normalize(lightVec);
        attenuation = 1.0 / (gConstant + gLinear * distance + gQuadratic * distance * distance);
    }
    else if (gLightType == 2) // Spot Light
    {
        float3 lightVec = gLightPosition - input.worldPos;
        float distance = length(lightVec);
        L = normalize(lightVec);

        // Attenuation
        attenuation = 1.0 / (gConstant + gLinear * distance + gQuadratic * distance * distance);

        // Spot light effect
        float theta = dot(L, normalize(-gLightDirection));
        float epsilon = gCutoffAngle - gOuterCutoffAngle;
        float intensity = clamp((theta - gOuterCutoffAngle) / epsilon, 0.0, 1.0);
        spotEffect = intensity;
    }

    // Diffuse lighting (Lambertian)
    float NdotL = max(dot(N, L), 0.0);
    float3 diffuse = NdotL * gLightColor * gLightIntensity * baseColor * attenuation * spotEffect;

    // Specular lighting (Blinn-Phong)
    float3 H = normalize(L + V); // Half vector for Blinn-Phong
    float spec = 0.0;
    if (NdotL > 0.0)
    {
        float NdotH = max(dot(N, H), 0.0);
        spec = pow(NdotH, 32.0); // Shininess = 32
    }
    float3 specular = spec * gLightColor * gLightIntensity * 0.5 * attenuation * spotEffect; // 0.5 = specular strength

    // Final color from primary light
    float3 finalColor = ambient + diffuse + specular;

    // Add secondary directional light if enabled
    if (gDirLightIntensity > 0.0)
    {
        float3 dirL = normalize(-gDirLightDirection);

        // Diffuse
        float dirNdotL = max(dot(N, dirL), 0.0);
        float3 dirDiffuse = dirNdotL * gDirLightColor * gDirLightIntensity * baseColor;

        // Specular
        float3 dirH = normalize(dirL + V);
        float dirSpec = 0.0;
        if (dirNdotL > 0.0)
        {
            float dirNdotH = max(dot(N, dirH), 0.0);
            dirSpec = pow(dirNdotH, 32.0);
        }
        float3 dirSpecular = dirSpec * gDirLightColor * gDirLightIntensity * 0.3; // Lower specular strength

        finalColor += dirDiffuse + dirSpecular;
    }

    return float4(finalColor, albedoColor.a);
}
