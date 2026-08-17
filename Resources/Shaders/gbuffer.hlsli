
cbuffer cbTransform : register(b0)
{
    row_major float4x4 gWorld;
    row_major float4x4 gView;
    row_major float4x4 gProjection;
};

cbuffer cbMaterial : register(b1)
{
    float4 albedoColor;
    float metallic;
    float roughness;
    float ao;
    float useAlbedoTexture; // 1.0 = sample t0; 0.0 = albedoColor only
    float4 emissiveColor;   // rgb = glTF emissiveFactor, scales tex_emissive; a unused
    float roughnessScale;   // multiplies the final roughness (texture or scalar); 1.0 = unchanged
    float metallicScale;    // multiplies the final metallic  (texture or scalar); 1.0 = unchanged
    float useARMTexture;
};

Texture2D tex_0 : register(t0); // Albedo (BaseColor)
Texture2D tex_1 : register(t1); // Normal map
Texture2D tex_arm : register(t2); // ARM
// t3 is reserved for instance data or the forward shadow map.
Texture2D tex_emissive : register(t4);

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

struct PS_MRT_OUTPUT
{
    float4 baseColor : SV_Target0;  // Base color (albedo)
    float4 normal : SV_Target1;     // World normal (encoded)
    float4 depth : SV_Target2;      // Depth value for visualization (R32_FLOAT writes to .r channel)
    float4 worldPos : SV_Target3;
    float4 arm : SV_Target4;        // ARM texture: R=AO, G=Roughness, B=Metallic
    float4 emissive : SV_Target5;   // Self-illumination, added unlit in deferred_lighting.hlsli
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

PS_MRT_OUTPUT PS_Main(VS_OUT input)
{
    PS_MRT_OUTPUT output;

    // Determine base color
    float3 baseColor;
    if (useAlbedoTexture > 0.5)
    {
        float4 texColor = tex_0.Sample(sam_0, input.uv);
        baseColor = texColor.rgb * albedoColor.rgb;
    }
    else
    {
        baseColor = albedoColor.rgb;
    }

    // Base Color (albedo)
    output.baseColor = float4(baseColor, albedoColor.a);

    // Normal (normalized and encoded to [0,1] range for storage)
    float3 normalSample = tex_1.Sample(sam_0, input.uv).rgb;
    float3 N; 
    if (dot(normalSample, normalSample) < 0.01)
    {
        N = normalize(input.normal);
    }
    else
    {
        float3 vN = normalize(input.normal);
        float3 up = abs(vN.y) < 0.999 ? float3(0, 1, 0) : float3(1, 0, 0);
        float3 vT = normalize(cross(up, vN));
        float3 vB = cross(vN, vT);

        float3 tsNormal = normalSample * 2.0 - 1.0;
        N = normalize(tsNormal.x * vT + tsNormal.y * vB + tsNormal.z * vN);
    }
    output.normal = float4(N * 0.5 + 0.5, 1.0); // Encode [-1,1] to [0,1]

    // ARM (AO, Roughness, Metallic)
    float4 armSample = tex_arm.Sample(sam_0, input.uv);
    float outAO, outRoughness, outMetallic;
    if (useARMTexture < 0.5)
    {
        outAO = ao; outRoughness = roughness; outMetallic = metallic;
    }
    else
    {
        outAO        = armSample.r;
        outRoughness = armSample.g;
        outMetallic  = armSample.b;
    }

    // Preserve texture variation while allowing per-material tuning.
    const float kMinRoughness = 0.045;
    output.arm = float4(outAO,
                        max(saturate(outRoughness * roughnessScale), kMinRoughness),
                        saturate(outMetallic  * metallicScale), 1.0);
    
    // Depth (R32_FLOAT only uses .r channel). Use the rasterizer's screen-space depth (SV_Position.z,
    // already [0,1] and perspective-correct) rather than interpolating a VS-computed z/w varying —
    // the latter is garbage for primitives spanning a large depth range (e.g. a 120 m lane line from
    // behind the camera to past the far plane), which made such geometry read as background and get
    // overwritten by the skybox resolve.
    output.depth = float4(input.pos.z, 0.0, 0.0, 1.0);

    // World Position
    output.worldPos = float4(input.worldPos, 1.0f);

    output.emissive = float4(tex_emissive.Sample(sam_0, input.uv).rgb * emissiveColor.rgb, 1.0);

    return output;
}
