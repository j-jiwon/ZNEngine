
cbuffer cbDebugView : register(b0)
{
    int viewType; // 0=Depth, 1=BaseColor, 2=Normal, 3=ShadowMap
    float3 padding;
};

Texture2D debugTexture : register(t0);
SamplerState debugSampler : register(s0);

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
    float4 texValue = debugTexture.Sample(debugSampler, input.uv);

    if (viewType == 0) // Depth
    {
        float depth = texValue.r;
        float near = 0.1f;
        float far = 100.0f;
        float linearDepth = (2.0f * near) / (far + near - depth * (far - near));
        linearDepth = pow(linearDepth, 0.4f);
        return float4(linearDepth, linearDepth, linearDepth, 1.0f);
    }
    else if (viewType == 1) // Base Color
    {
        // Display base color directly
        return float4(texValue.rgb, 1.0f);
    }
    else if (viewType == 2) // Normal
    {
        // Decode normal from [0,1] back to [-1,1] and remap to [0,1] for display
        float3 normal = texValue.rgb * 2.0f - 1.0f; // Decode
        normal = normalize(normal);
        normal = normal * 0.5f + 0.5f; // Remap to [0,1] for visualization
        return float4(normal, 1.0f);
    }
    else if (viewType == 3) // ShadowMap
    {
        // Shadow map stores depth values [0,1]
        float shadowDepth = texValue.r;
        // Apply gamma for better visibility
        shadowDepth = pow(shadowDepth, 0.5f);
        return float4(shadowDepth, shadowDepth, shadowDepth, 1.0f);
    }

    return float4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta for error
}
