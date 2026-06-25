// Forward PBR shader for offscreen camera passes (CCTV, etc.)
// Uses the scene's actual directional light and spotlights from cbForwardLight (b2).
// Shadow map bound at t3; PCF 3x3 using sampler0.

#define FWD_MAX_SPOTS 2

struct FwdSpotData
{
    float3 position;
    float  intensity;
    float3 direction;
    float  innerCutoff;
    float3 color;
    float  outerCutoff;
    float  attConst;     // renamed: 'constant' and 'linear' are HLSL keywords
    float  attLinear;
    float  attQuad;
    float  padding;
};

cbuffer cbTransform : register(b0)
{
    row_major float4x4 gWorld;
    row_major float4x4 gView;
    row_major float4x4 gProjection;
};

cbuffer cbMaterial : register(b1)
{
    float4 albedoColor;
    float  metallic;
    float  roughness;
    float  ao;
    float  matPad;
};

cbuffer cbForwardLight : register(b2)
{
    float3 fwdDirLightDir;         float fwdDirLightIntensity;    // 16
    float3 fwdDirLightColor;       float fwdDirAmbientIntensity;  // 16
    float3 fwdViewPosition;        int   fwdNumSpotLights;        // 16
    FwdSpotData fwdSpots[FWD_MAX_SPOTS];                          // 128 (2 × 64)
    row_major float4x4 fwdLightViewProj;                          // 64
    float fwdShadowBias; float fwdShadowMapW; float fwdShadowMapH; float fwdShadowPad; // 16
    // Total: 256 bytes
};

Texture2D<float> fwdShadowMap : register(t3);
SamplerState     sampler0     : register(s0);

struct VS_IN
{
    float3 pos    : POSITION;
    float4 color  : COLOR;
    float2 uv     : TEXCOORD;
    float3 normal : NORMAL;
};

struct VS_OUT
{
    float4 pos         : SV_Position;
    float3 worldPos    : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;
    float4 wp          = mul(float4(input.pos, 1.f), gWorld);
    output.pos         = mul(mul(wp, gView), gProjection);
    output.worldPos    = wp.xyz;
    output.worldNormal = mul(float4(input.normal, 0.f), gWorld).xyz;
    return output;
}

// ---- Shadow PCF (3×3, sampler0 point-sample + manual compare) ----------

float FwdShadow(float3 worldPos, float3 N, float3 L)
{
    if (fwdShadowMapW <= 0.f) return 1.f; // no shadow map
    float4 sp = mul(float4(worldPos, 1.f), fwdLightViewProj);
    sp.xyz /= sp.w;
    // Clip-space bounds check
    if (sp.x < -1.f || sp.x > 1.f || sp.y < -1.f || sp.y > 1.f || sp.z < 0.f || sp.z > 1.f)
        return 1.f;
    float2 uv    = float2(sp.x * 0.5f + 0.5f, -sp.y * 0.5f + 0.5f);
    float  bias  = max(fwdShadowBias * (1.f - dot(N, L)), fwdShadowBias * 0.1f);
    float  depth = sp.z - bias;
    float2 texel = 1.f / float2(fwdShadowMapW, fwdShadowMapH);
    float  shadow = 0.f;
    [unroll] for (int xi = -1; xi <= 1; ++xi)
    [unroll] for (int yi = -1; yi <= 1; ++yi)
        shadow += (depth < fwdShadowMap.SampleLevel(sampler0, uv + float2(xi, yi) * texel, 0).r) ? 1.f : 0.f;
    return shadow / 9.f;
}

// ---- Cook-Torrance PBR helpers ----------------------------------------

static const float FWD_PI = 3.14159265359f;

float FwdDistGGX(float NdotH, float roughSq)
{
    float a2   = roughSq * roughSq;
    float denom = NdotH * NdotH * (a2 - 1.f) + 1.f;
    return a2 / max(FWD_PI * denom * denom, 0.0001f);
}

float FwdGeoSmith(float NdotV, float NdotL, float roughSq)
{
    float r1 = (roughSq + 1.f) * (roughSq + 1.f) / 8.f;
    float gV = NdotV / max(NdotV * (1.f - r1) + r1, 0.0001f);
    float gL = NdotL / max(NdotL * (1.f - r1) + r1, 0.0001f);
    return gV * gL;
}

float3 FwdFresnel(float cosTheta, float3 F0)
{
    return F0 + (1.f - F0) * pow(clamp(1.f - cosTheta, 0.f, 1.f), 5.f);
}

float3 FwdPBR(float3 N, float3 V, float3 L, float3 radiance,
              float3 albedo, float met, float rough)
{
    float3 H     = normalize(V + L);
    float  NdotV = max(dot(N, V), 0.f);
    float  NdotL = max(dot(N, L), 0.f);
    float  NdotH = max(dot(N, H), 0.f);
    float  HdotV = max(dot(H, V), 0.f);
    float  roughSq = rough * rough;

    float3 F0  = lerp(float3(0.04f, 0.04f, 0.04f), albedo, met);
    float  NDF = FwdDistGGX(NdotH, roughSq);
    float  G   = FwdGeoSmith(NdotV, NdotL, roughSq);
    float3 F   = FwdFresnel(HdotV, F0);

    float3 spec = (NDF * G * F) / max(4.f * NdotV * NdotL + 0.0001f, 0.0001f);
    float3 kD   = (float3(1.f, 1.f, 1.f) - F) * (1.f - met);
    return (kD * albedo / FWD_PI + spec) * radiance * NdotL;
}

// -----------------------------------------------------------------------

float4 PS_Main(VS_OUT input) : SV_Target
{
    float3 albedo = albedoColor.rgb;
    float  met    = metallic;
    float  rough  = max(roughness, 0.05f);
    float  aoVal  = max(ao, 0.1f);

    float3 N = normalize(input.worldNormal);
    float3 V = normalize(fwdViewPosition - input.worldPos);

    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * aoVal * fwdDirAmbientIntensity;
    float3 Lo      = float3(0.f, 0.f, 0.f);

    // Directional light (with shadow)
    if (fwdDirLightIntensity > 0.f)
    {
        float3 Ldir   = normalize(-fwdDirLightDir);
        float  shadow = FwdShadow(input.worldPos, N, Ldir);
        Lo += FwdPBR(N, V, Ldir, fwdDirLightColor * fwdDirLightIntensity * shadow, albedo, met, rough);
    }

    // Spotlights
    for (int i = 0; i < fwdNumSpotLights; ++i)
    {
        FwdSpotData s  = fwdSpots[i];
        float3 toL     = s.position - input.worldPos;
        float  dist    = length(toL);
        float3 Lspot   = normalize(toL);
        float  att     = 1.f / max(s.attConst + s.attLinear * dist + s.attQuad * dist * dist, 0.0001f);
        float  theta   = dot(Lspot, normalize(-s.direction));
        float  eps     = s.innerCutoff - s.outerCutoff;
        float  spotFac = clamp((theta - s.outerCutoff) / max(eps, 0.0001f), 0.f, 1.f);
        Lo += FwdPBR(N, V, Lspot, s.color * s.intensity * att * spotFac, albedo, met, rough);
    }

    return float4(ambient + Lo, 1.f);
}
