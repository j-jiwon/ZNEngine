
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
};

Texture2D tex_0 : register(t0); // Albedo (BaseColor)
Texture2D tex_1 : register(t1); // Normal map
Texture2D tex_arm : register(t2); // ARM

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
    float depth : DEPTH;
};

struct PS_MRT_OUTPUT
{
    float4 baseColor : SV_Target0;  // Base color (albedo)
    float4 normal : SV_Target1;     // World normal (encoded)
    float4 depth : SV_Target2;      // Depth value for visualization (R32_FLOAT writes to .r channel)
    float4 worldPos : SV_Target3;
    float4 arm : SV_Target4;        // ARM texture: R=AO, G=Roughness, B=Metallic
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

    // Calculate linear depth (0 = near, 1 = far)
    output.depth = output.pos.z / output.pos.w;

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
    if (dot(armSample.rgb, armSample.rgb) < 0.01)
    {
        output.arm = float4(ao, roughness, metallic, 1.0);
    }
    else
    {
        // Use texture values directly (no conversion)
        output.arm = float4(armSample.r, armSample.g, armSample.b, 1.0);
    }
    
    // Depth (R32_FLOAT only uses .r channel)
    output.depth = float4(input.depth, 0.0, 0.0, 1.0);

    // World Position
    output.worldPos = float4(input.worldPos, 1.0f);

    return output;
}
