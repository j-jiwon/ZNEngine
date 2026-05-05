
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

float grid(float2 coord, float gridSize, float lineWidth)
{
    // Simple grid pattern using modulo
    float2 gridCoord = abs(frac(coord / gridSize) - 0.5);
    float2 gridLine = step(gridCoord, float2(lineWidth / gridSize, lineWidth / gridSize));
    return max(gridLine.x, gridLine.y);
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    // Grid parameters
    float gridSize = 5.0; // Grid cell size in world units
    float lineWidth = 0.01; // Line width in world units
    float3 gridColor = float3(0.8, 0.8, 0.8); // Grid line color (light gray)
    float3 backgroundColor = float3(0.1, 0.1, 0.1); // Background color (dark gray)

    // Calculate grid on XZ plane for horizontal floor
    float2 gridCoord = input.worldPos.xz;
    float gridPattern = grid(gridCoord, gridSize, lineWidth);

    // Axis lines (X and Z axes for horizontal plane)
    float axisLineWidth = 0.02;
    float xAxis = smoothstep(axisLineWidth, 0.0, abs(input.worldPos.z));
    float zAxis = smoothstep(axisLineWidth, 0.0, abs(input.worldPos.x));

    // Combine grid with axis lines
    float3 baseColor = lerp(backgroundColor, gridColor, gridPattern);
    baseColor = lerp(baseColor, float3(1.0, 0.0, 0.0), xAxis); // Red for X axis
    baseColor = lerp(baseColor, float3(0.0, 0.0, 1.0), zAxis); // Blue for Z axis

    // Alpha: grid lines and axis lines are opaque, background is transparent
    float isLine = max(gridPattern, max(xAxis, zAxis));
    float alpha = lerp(0.0, 1.0, isLine); // Background 30% opacity, lines 100%

    // Apply albedo color tint
    baseColor *= albedoColor.rgb;

    // Normalize interpolated normal
    float3 N = normalize(input.normal);
    float3 V = normalize(gCameraPos - input.worldPos); // View direction

    // High ambient lighting to make grid always visible
    float3 ambient = baseColor * 0.8;

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

    // Diffuse lighting (Lambertian) - reduced contribution
    float NdotL = max(dot(N, L), 0.0);
    float3 diffuse = NdotL * gLightColor * gLightIntensity * baseColor * attenuation * spotEffect * 0.2;

    // Final color from primary light
    float3 finalColor = ambient + diffuse;

    // Add secondary directional light if enabled
    if (gDirLightIntensity > 0.0)
    {
        float3 dirL = normalize(-gDirLightDirection);

        // Diffuse
        float dirNdotL = max(dot(N, dirL), 0.0);
        float3 dirDiffuse = dirNdotL * gDirLightColor * gDirLightIntensity * baseColor * 0.2;

        finalColor += dirDiffuse;
    }

    return float4(finalColor, alpha);
}
