
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

float grid(float2 worldXZ, float gridSize, float3 worldPos, float3 cameraPos)
{    
    float2 coord = worldXZ / gridSize;
    float2 deriv = fwidth(coord); // 현재 픽셀의 미분값 (화면 공간 기준)
    float2 gridLines = abs(frac(coord - 0.5) - 0.5) / deriv;
    float lineVal = min(gridLines.x, gridLines.y);
    
    return 1.0 - min(lineVal, 1.0);
}


float4 PS_Main(VS_OUT input) : SV_Target
{
    float dist = length(input.worldPos - gCameraPos);

    // 거리에 따라 3단계 grid 크기
    float g1 = grid(input.worldPos.xz, 1.0, input.worldPos, gCameraPos);
    float g2 = grid(input.worldPos.xz, 5.0, input.worldPos, gCameraPos);
    float g3 = grid(input.worldPos.xz, 25.0, input.worldPos, gCameraPos);

    // 거리별 blend
    float t1 = smoothstep(0.0, 30.0, dist); // 0~20: g1→g2
    float t2 = smoothstep(20.0, 80.0, dist); // 15~60: g2→g3

    float gridPattern = lerp(g1, lerp(g2, g3, t2), t1);

    // axis lines
    float xAxisDeriv = fwidth(input.worldPos.z);
    float zAxisDeriv = fwidth(input.worldPos.x);
    float axisWidth = 0.08;
    float xAxis = 1.0 - smoothstep(0.0, axisWidth + xAxisDeriv * 2.0, abs(input.worldPos.z));
    float zAxis = 1.0 - smoothstep(0.0, axisWidth + zAxisDeriv * 2.0, abs(input.worldPos.x));

    float3 baseColor = lerp(float3(0.1, 0.1, 0.1), float3(0.8, 0.8, 0.8), gridPattern);
    baseColor = lerp(baseColor, float3(1.0, 0.0, 0.0), xAxis);
    baseColor = lerp(baseColor, float3(0.0, 0.0, 1.0), zAxis);

    float isLine = max(gridPattern, max(xAxis, zAxis));
    float fade = 1.0 - saturate(dist / 60.0);

    return float4(baseColor * albedoColor.rgb, isLine * fade);
}
