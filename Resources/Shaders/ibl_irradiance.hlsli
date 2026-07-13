// Diffuse irradiance convolution: one draw per cube face. Reconstructs the world-space
// direction for each pixel of the destination face from a per-face basis (forward/right/up,
// same basis ZNScene::AddCubemapCapture uses per face), then cosine-weight integrates the
// source environment cube over the hemisphere around that direction (LearnOpenGL/Lagarde
// convolution, standard low-frequency diffuse irradiance approximation).

cbuffer cbFace : register(b0)
{
    float3 faceForward; float _pad0;
    float3 faceRight;   float _pad1;
    float3 faceUp;      float _pad2;
};

TextureCube srcEnvCube : register(t0);
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

static const float PI = 3.14159265359f;

float4 PS_Main(VS_OUT input) : SV_Target
{
    float2 ndc = input.uv * 2.0f - 1.0f;
    ndc.y = -ndc.y;
    float3 N = normalize(faceForward + ndc.x * faceRight + ndc.y * faceUp);

    float3 up = (abs(N.y) < 0.999f) ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 right = normalize(cross(up, N));
    up = cross(N, right);

    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    float sampleCount = 0.0f;
    float delta = 0.025f;

    for (float phi = 0.0f; phi < 2.0f * PI; phi += delta)
    {
        for (float theta = 0.0f; theta < 0.5f * PI; theta += delta)
        {
            // Spherical -> tangent-space Cartesian, then into world space around N
            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            irradiance += srcEnvCube.Sample(sampler0, sampleVec).rgb * cos(theta) * sin(theta);
            sampleCount += 1.0f;
        }
    }

    irradiance = PI * irradiance / max(sampleCount, 1.0f);
    return float4(irradiance, 1.0f);
}
