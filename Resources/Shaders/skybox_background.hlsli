// Cube-capture skybox background fill: drawn once per face, before any scene geometry,
// into a room-reflection capture face (see ZNScene::AddCubemapCapture / CubeCapturePass).
// Unlike skybox_resolve.hlsli there's no depth test — this always fills the whole face;
// subsequent depth-tested scene geometry draws naturally overwrite it where objects exist.
// Each face's direction is fixed (same 90-degree/1:1 aspect basis as the capture cameras
// themselves), so forward/right/up are constants per draw, no FOV/tan needed.

cbuffer cbSkybox : register(b0)
{
    float3 forward; float _pad0;
    float3 right;   float _pad1;
    float3 up;      float _pad2;
};

TextureCube skyboxCube : register(t0);
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

float4 PS_Main(VS_OUT input) : SV_Target
{
    float2 ndc = input.uv * 2.0f - 1.0f;
    ndc.y = -ndc.y;
    float3 rayDir = normalize(forward + ndc.x * right + ndc.y * up);
    return float4(skyboxCube.Sample(sampler0, rayDir).rgb, 1.0f);
}
