
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
    float depth : DEPTH;
};

struct PS_MRT_OUTPUT
{
    float4 baseColor : SV_Target0;  // Base color (albedo)
    float4 normal : SV_Target1;     // World normal (encoded)
    float4 depth : SV_Target2;      // Depth value for visualization (R32_FLOAT writes to .r channel)
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

    // Output 0: Base Color (albedo)
    output.baseColor = float4(baseColor, albedoColor.a);

    // Output 1: World Normal (normalized and encoded to [0,1] range for storage)
    float3 N = normalize(input.normal);
    output.normal = float4(N * 0.5 + 0.5, 1.0); // Encode [-1,1] to [0,1]

    // Output 2: Depth for visualization (R32_FLOAT only uses .r channel)
    output.depth = float4(input.depth, 0.0, 0.0, 1.0);

    return output;
}
