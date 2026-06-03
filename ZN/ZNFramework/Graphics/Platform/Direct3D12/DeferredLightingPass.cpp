#include "DeferredLightingPass.h"
#include "GBufferManager.h"
#include "ShadowMap.h"
#include "Shader.h"
#include "GraphicsDevice.h"
#include "CommandQueue.h"
#include "DirectionalLight.h"
#include "ZNFramework.h"
#include "../../ZNLight.h"
#include "../../../Math/ZNVector3.h"
#include "../../../ZNCamera.h"

using namespace ZNFramework;

struct LightingVertex
{
    float pos[3];
    float color[4];
    float uv[2];
    float normal[3];
};

#define MAX_SPOT_LIGHTS 8

struct SpotLightData
{
    float position[3];
    float intensity;
    float direction[3];
    float innerCutoff;
    float color[3];
    float outerCutoff;
    float attenuationConstant;
    float attenuationLinear;
    float attenuationQuadratic;
    float padding;
};

struct DeferredLightCB
{
    // Directional Light
    float dirLightDirection[3];
    float dirLightIntensity;
    float dirLightColor[3];
    float dirAmbientIntensity;

    // Camera
    float viewPosition[3];
    int numSpotLights;

    // Shadow mapping
    float lightViewProj[16];
    float shadowMapSize[2];
    float shadowBias;
    float shadowPCFRadius;

    // Spot Lights array
    SpotLightData spotLights[MAX_SPOT_LIGHTS];
};

void DeferredLightingPass::Init()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    CreateFullscreenQuad();

    // Load deferred lighting shader
    lightingShader = new Shader();
    std::filesystem::path lightingShaderPath = GetResourcePath() / L"Shaders" / L"deferred_lighting.hlsli";
    lightingShader->Load(lightingShaderPath);
    lightingShader->DisableDepthTest();

    // Create constant buffer for lighting
    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    uint32 lightingBufferSize = (sizeof(DeferredLightCB) + 255) & ~255;
    D3D12_RESOURCE_DESC lightingBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(lightingBufferSize);

    ThrowIfFailed(device->Device()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &lightingBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&lightingConstantBuffer)
    ));

    lightingConstantBuffer->Map(0, nullptr, &mappedLightingBuffer);

    // Create descriptor heap for lighting pass
    D3D12_DESCRIPTOR_HEAP_DESC lightingHeapDesc = {};
    lightingHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    lightingHeapDesc.NumDescriptors = 12; // b0~b4 (5) + t0~t5 (7, including shadow map)
    lightingHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(device->Device()->CreateDescriptorHeap(&lightingHeapDesc, IID_PPV_ARGS(&lightingDescriptorHeap)));
}

void DeferredLightingPass::CreateFullscreenQuad()
{
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    LightingVertex vertices[] = {
        { {-1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
        { { 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
        { {-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} },
        { { 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }
    };

    uint32 bufferSize = sizeof(vertices);

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    ThrowIfFailed(device->Device()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&quadVertexBuffer)
    ));

    void* mappedData = nullptr;
    quadVertexBuffer->Map(0, nullptr, &mappedData);
    memcpy(mappedData, vertices, bufferSize);
    quadVertexBuffer->Unmap(0, nullptr);

    quadVertexBufferView.BufferLocation = quadVertexBuffer->GetGPUVirtualAddress();
    quadVertexBufferView.SizeInBytes = bufferSize;
    quadVertexBufferView.StrideInBytes = sizeof(LightingVertex);
}

void DeferredLightingPass::Render(GBufferManager* gbufferManager, ShadowMap* shadowMap, uint32 screenWidth, uint32 screenHeight)
{
    if (!gbufferManager)
        return;

    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
    CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
    ID3D12GraphicsCommandList* cmdList = queue->CommandList();

    // Update lighting constant buffer
    DeferredLightCB lightData = {};

    ZNDirectionalLight* dirLight = GraphicsContext::GetInstance().GetDirectionalLight();
    const auto& spotLights = GraphicsContext::GetInstance().GetSpotLights();
    ZNCamera* camera = GraphicsContext::GetInstance().GetCamera();

    if (dirLight)
    {
        ZNVector3 dir = dirLight->GetDirection();
        ZNVector3 color = dirLight->GetColor();
        lightData.dirLightDirection[0] = dir.x;
        lightData.dirLightDirection[1] = dir.y;
        lightData.dirLightDirection[2] = dir.z;
        lightData.dirLightIntensity = dirLight->GetIntensity();
        lightData.dirLightColor[0] = color.x;
        lightData.dirLightColor[1] = color.y;
        lightData.dirLightColor[2] = color.z;
        lightData.dirAmbientIntensity = dirLight->GetAmbientIntensity();

        // Shadow mapping data
        Platform::Direct3D::DirectionalLight* d3dDirLight = dynamic_cast<Platform::Direct3D::DirectionalLight*>(dirLight);
        if (d3dDirLight && shadowMap)
        {
            ZNMatrix4 lightVP = d3dDirLight->GetLightViewProjectionMatrix();
            memcpy(lightData.lightViewProj, lightVP.value, sizeof(float) * 16);
            lightData.shadowMapSize[0] = static_cast<float>(shadowMap->GetWidth());
            lightData.shadowMapSize[1] = static_cast<float>(shadowMap->GetHeight());
            lightData.shadowBias = 0.005f;
            lightData.shadowPCFRadius = 1.0f;
        }
    }

    // Fill spotlight array
    int numSpots = 0;
    for (size_t i = 0; i < spotLights.size() && i < MAX_SPOT_LIGHTS; ++i)
    {
        ZNLight* light = spotLights[i];
        if (light && light->GetType() == LightType::Spot)
        {
            ZNSpotLight* spotLight = static_cast<ZNSpotLight*>(light);
            ZNVector3 pos = spotLight->GetPosition();
            ZNVector3 dir = spotLight->GetDirection();
            ZNVector3 color = spotLight->GetColor();

            SpotLightData& data = lightData.spotLights[numSpots];
            data.position[0] = pos.x;
            data.position[1] = pos.y;
            data.position[2] = pos.z;
            data.intensity = spotLight->GetIntensity();
            data.direction[0] = dir.x;
            data.direction[1] = dir.y;
            data.direction[2] = dir.z;
            data.innerCutoff = cos(spotLight->GetInnerCutoffAngle() * 3.14159f / 180.0f);
            data.color[0] = color.x;
            data.color[1] = color.y;
            data.color[2] = color.z;
            data.outerCutoff = cos(spotLight->GetOuterCutoffAngle() * 3.14159f / 180.0f);

            data.attenuationConstant = spotLight->GetConstantAttenuation();
            data.attenuationLinear = spotLight->GetLinearAttenuation();
            data.attenuationQuadratic = spotLight->GetQuadraticAttenuation();

            ++numSpots;
        }
    }
    lightData.numSpotLights = numSpots;

    if (camera)
    {
        ZNVector3 camPos = camera->GetPosition();
        lightData.viewPosition[0] = camPos.x;
        lightData.viewPosition[1] = camPos.y;
        lightData.viewPosition[2] = camPos.z;
    }

    memcpy(mappedLightingBuffer, &lightData, sizeof(DeferredLightCB));

    // Set fullscreen viewport
    D3D12_VIEWPORT viewport = { 0, 0, static_cast<FLOAT>(screenWidth), static_cast<FLOAT>(screenHeight), 0.f, 1.f };
    D3D12_RECT scissorRect = CD3DX12_RECT(0, 0, screenWidth, screenHeight);
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissorRect);

    if (lightingShader)
    {
        lightingShader->Bind();
    }

    uint32 lightingDescSize = device->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create CBV for lighting at b0
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = lightingDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = lightingConstantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = lightingConstantBuffer->GetDesc().Width;
    device->Device()->CreateConstantBufferView(&cbvDesc, cpuHandle);

    // Copy G-Buffer SRVs
    cpuHandle.ptr += lightingDescSize * 5;
    device->Device()->CopyDescriptorsSimple(1, cpuHandle, gbufferManager->GetBaseColorSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cpuHandle.ptr += lightingDescSize;
    device->Device()->CopyDescriptorsSimple(1, cpuHandle, gbufferManager->GetNormalSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cpuHandle.ptr += lightingDescSize;
    device->Device()->CopyDescriptorsSimple(1, cpuHandle, gbufferManager->GetDepthCopySRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cpuHandle.ptr += lightingDescSize;
    device->Device()->CopyDescriptorsSimple(1, cpuHandle, gbufferManager->GetWorldPosSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cpuHandle.ptr += lightingDescSize;
    device->Device()->CopyDescriptorsSimple(1, cpuHandle, gbufferManager->GetARMSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Copy Shadow Map SRV (t5)
    if (shadowMap)
    {
        cpuHandle.ptr += lightingDescSize;
        device->Device()->CopyDescriptorsSimple(1, cpuHandle, shadowMap->GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    // Set descriptor heap
    ID3D12DescriptorHeap* heaps[] = { lightingDescriptorHeap.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = lightingDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    cmdList->SetGraphicsRootDescriptorTable(0, gpuHandle);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmdList->IASetVertexBuffers(0, 1, &quadVertexBufferView);

    cmdList->DrawInstanced(4, 1, 0, 0);
}
