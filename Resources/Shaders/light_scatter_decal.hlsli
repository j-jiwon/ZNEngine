// Procedural floor decal standing in for light bouncing off small mirror facets
// (mirror-ball body, monster's mirror-tile patches) — not physically traced, just a
// radial cluster of colored glints tinted by the room's 4 spotlight colors. Reused for
// both sources by repurposing cbMaterial's PBR fields as decal params (this decal isn't
// lit, so metallic/roughness/ao have no PBR meaning here):
//   metallic = dot count per color layer
//   roughness = overall intensity multiplier
//   ao        = dot radius (in UV units)

cbuffer cbTransform : register(b0)
{
    row_major float4x4 gWorld;
    row_major float4x4 gView;
    row_major float4x4 gProjection;
};

cbuffer cbMaterial : register(b1)
{
    float4 albedoColor; // unused
    float  metallic;    // dot count per layer
    float  roughness;   // intensity multiplier
    float  ao;          // dot radius
    float  padding;
};

struct VS_IN
{
    float3 pos    : POSITION;
    float4 color  : COLOR;
    float2 uv     : TEXCOORD;
    float3 normal : NORMAL;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;
    float4 worldPos = mul(float4(input.pos, 1.f), gWorld);
    float4 viewPos  = mul(worldPos, gView);
    output.pos = mul(viewPos, gProjection);
    output.uv  = input.uv;
    return output;
}

static const float3 kLightColors[4] = {
    float3(1.000f, 0.176f, 0.825f), // pink
    float3(0.949f, 0.865f, 0.147f), // yellow
    float3(0.143f, 0.843f, 0.922f), // cyan
    float3(0.906f, 0.467f, 0.906f), // purple
};

float2 Hash2(float2 p)
{
    p = float2(dot(p, float2(127.1f, 311.7f)), dot(p, float2(269.5f, 183.3f)));
    return frac(sin(p) * 43758.5453123f);
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    float2 fromCenter = input.uv - 0.5f;
    float  edgeFade   = saturate(1.f - length(fromCenter) * 2.f);
    edgeFade *= edgeFade;

    int   dotCount  = (int)metallic;
    float dotRadius = ao;

    float3 accum = float3(0.f, 0.f, 0.f);
    for (int layer = 0; layer < 4; ++layer)
    {
        for (int i = 0; i < dotCount; ++i)
        {
            float2 seed   = float2((float)(layer * 97 + i), (float)(i * 13 + layer * 7));
            float2 dotPos = Hash2(seed);
            float  d      = distance(input.uv, dotPos);
            float  glint  = saturate(1.f - d / dotRadius);
            glint = glint * glint * glint;
            accum += kLightColors[layer] * glint;
        }
    }

    accum *= edgeFade * roughness;
    return float4(accum, 1.f);
}
