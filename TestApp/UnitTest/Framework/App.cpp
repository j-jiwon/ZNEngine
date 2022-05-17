
#include "gtest/gtest.h"
#include "ZNFramework.h"

using namespace ZNFramework;

class TestApp : public ZNApplication
{
public:
    TestApp(HINSTANCE hInstance);
    TestApp(const TestApp& rhs) = delete;
    TestApp& operator=(const TestApp& rhs) = delete;
    ~TestApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const ZNTimer& gt)override;
    virtual void Draw(const ZNTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
};


TEST(Application, Create)
{
	try
	{
        TestApp app;
        app.Run();

		EXPECT_EQ(true, true);
	}
	catch(...)
	{
		EXPECT_EQ(true, false);
	}
}


/*
#include "ZNFramework.h"
#include <DirectXColors.h>
#include <windows.h>
#include <array>


using Microsoft::WRL::ComPtr;
using namespace DirectX;

using namespace ZNFramework;

# define PI 3.1415926535f;
# define clamp(x, low, high) ( x < low ? low : (x > high ? high : x))

class TestApp : public ZNApplication
{
public:
    TestApp(HINSTANCE hInstance);
    TestApp(const TestApp& rhs) = delete;
    TestApp& operator=(const TestApp& rhs) = delete;
    ~TestApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const ZNTimer& gt)override;
    virtual void Draw(const ZNTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void BuildDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPSO();

private:
    ComPtr<ID3D12RootSignature> rootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> cbvHeap = nullptr;

    std::unique_ptr<ZNUtils::UploadBuffer<ObjectConstants>> objectConstantBuffer = nullptr;
    std::unique_ptr<MeshGeometry> boxGeometry = nullptr;

    ComPtr<ID3DBlob> vsByteCode = nullptr;
    ComPtr<ID3DBlob> psByteCode = nullptr;

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;

    ComPtr<ID3D12PipelineState> PSO = nullptr;

    XMFLOAT4X4 worldMatrix = VectorMath::Identity4X4();
    XMFLOAT4X4 viewMatrix = VectorMath::Identity4X4();
    XMFLOAT4X4 projectMatrix = VectorMath::Identity4X4();

    float theta = 1.5f * XM_PI;
    float piDiv4 = XM_PIDIV4;
    float radius = 5.0f;

    POINT lastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        TestApp theApp(hInstance);
        if (!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch (...)
    {
        MessageBox(nullptr, L"", L"HR Failed", MB_OK);
        return 0;
    }
}

TestApp::TestApp(HINSTANCE hInstance)
    : ZNApplication(hInstance)
{
}

TestApp::~TestApp()
{
}

bool TestApp::Initialize()
{
    if (!ZNApplication::Initialize())
        return false;

    ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

    BuildDescriptorHeaps();
    BuildConstantBuffers();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildBoxGeometry();
    BuildPSO();

    ThrowIfFailed(commandList->Close());
    ID3D12CommandList* cmdsLists[]{ commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    FlushCommandQueue();

    return true;
}

void TestApp::OnResize()
{
    ZNApplication::OnResize();
    XMMATRIX p{ XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f) };
    XMStoreFloat4x4(&projectMatrix, p);
}

void TestApp::Update(const ZNTimer& gt)
{
    // Convert Spherical to Cartesian coordinates.
    float x = radius * sinf(piDiv4) * cosf(theta);
    float z = radius * sinf(piDiv4) * sinf(theta);
    float y = radius * cosf(piDiv4);

    // Build the view matrix.
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&viewMatrix, view);

    XMMATRIX world = XMLoadFloat4x4(&worldMatrix);
    XMMATRIX proj = XMLoadFloat4x4(&projectMatrix);
    XMMATRIX worldViewProj = world * view * proj;

    // Update the constant buffer with the latest worldViewProj matrix.
    ObjectConstants objConstants;
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
    objectConstantBuffer->CopyData(0, objConstants);
}

void TestApp::Draw(const ZNTimer& gt)
{
    ThrowIfFailed(commandAllocator->Reset());
    ThrowIfFailed(commandList->Reset(commandAllocator.Get(), PSO.Get()));

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Transition.pResource = CurrentBackBuffer();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    commandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::GreenYellow, 0, nullptr);
    commandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    auto currentBackBufferView = CurrentBackBufferView();
    auto depthStencilView = DepthStencilView();
    commandList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthStencilView);

    ID3D12DescriptorHeap* descriptorHeaps[] = { cbvHeap.Get() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootSignature(rootSignature.Get());

    auto vertexBufferView = boxGeometry->VertexBufferView();
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    auto indexBufferView = boxGeometry->IndexBufferView();
    commandList->IASetIndexBuffer(&indexBufferView);
    commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->SetGraphicsRootDescriptorTable(0, cbvHeap->GetGPUDescriptorHandleForHeapStart());

    commandList->DrawIndexedInstanced(
        boxGeometry->DrawArgs["box"].IndexCount,
        1, 0, 0, 0);


    barrier.Transition.pResource = CurrentBackBuffer();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList->ResourceBarrier(1, &barrier);

    ThrowIfFailed(commandList->Close());

    ID3D12CommandList* cmdsLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    ThrowIfFailed(swapChain->Present(0, 0));
    currBackBuffer = (currBackBuffer + 1) % SwapChainBufferCount;

    FlushCommandQueue();
}


void TestApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    lastMousePos.x = x;
    lastMousePos.y = y;

    SetCapture(hwnd);
}

void TestApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void TestApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        // cube를 잡고 움직이는 형태가 되도록 함 
        float dx = XMConvertToRadians(0.25f * static_cast<float>(lastMousePos.x - x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(lastMousePos.y - y));

        // Update angles based on input to orbit camera around box.
        theta += dx;
        piDiv4 += dy;

        // Restrict the angle mPhi.
        piDiv4 = clamp(piDiv4, 0.1f, 3.1415926535f - 0.1f);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.005 unit in the scene.
        float dx = 0.005f * static_cast<float>(x - lastMousePos.x);
        float dy = 0.005f * static_cast<float>(y - lastMousePos.y);

        // Update the camera radius based on input.
        radius += dx - dy;

        // Restrict the radius.
        radius = clamp(radius, 3.0f, 15.0f);
    }

    lastMousePos.x = x;
    lastMousePos.y = y;
}

void TestApp::BuildDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(graphicDevice->CreateDescriptorHeap(&cbvHeapDesc,
        IID_PPV_ARGS(&cbvHeap)));
}

void TestApp::BuildConstantBuffers()
{
    objectConstantBuffer = std::make_unique<ZNUtils::UploadBuffer<ObjectConstants>>(graphicDevice.Get(), 1, true);

    constexpr UINT objCBByteSize{ ZNUtils::CalcConstantBufferByteSize(sizeof(ObjectConstants)) };

    D3D12_GPU_VIRTUAL_ADDRESS cbAddress{ objectConstantBuffer->GetResource()->GetGPUVirtualAddress() };
    // Offset to the ith object constant buffer in the buffer.
    int boxCBufIndex = 0;
    cbAddress += boxCBufIndex * objCBByteSize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes = objCBByteSize;

    graphicDevice->CreateConstantBufferView(
        &cbvDesc,
        cbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void TestApp::BuildRootSignature()
{
    D3D12_ROOT_PARAMETER  slotRootParameter[1]{};

    D3D12_DESCRIPTOR_RANGE   cbvTable{};
    cbvTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    cbvTable.NumDescriptors = 1;
    cbvTable.BaseShaderRegister = 0;

    slotRootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    slotRootParameter[0].DescriptorTable.NumDescriptorRanges = 1;
    slotRootParameter[0].DescriptorTable.pDescriptorRanges = &cbvTable;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.NumParameters = 1;
    rootSigDesc.pParameters = slotRootParameter;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr{ D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()) };

    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    graphicDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature));
}

void TestApp::BuildShadersAndInputLayout()
{
    HRESULT hr = S_OK;

    vsByteCode = ZNUtils::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
    psByteCode = ZNUtils::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

    inputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void TestApp::BuildBoxGeometry()
{
    std::array<Vertex, 8> vertices =
    {
        Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
        Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
        Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
        Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
        Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
        Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
        Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
        Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
    };

    std::array<std::uint16_t, 36> indices =
    {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 6, 5,
        4, 7, 6,

        // left face
        4, 5, 1,
        4, 1, 0,

        // right face
        3, 2, 6,
        3, 6, 7,

        // top face
        1, 5, 6,
        1, 6, 2,

        // bottom face
        4, 0, 3,
        4, 3, 7
    };

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    boxGeometry = std::make_unique<MeshGeometry>();
    boxGeometry->Name = "boxGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &boxGeometry->VertexBufferCPU));
    CopyMemory(boxGeometry->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &boxGeometry->IndexBufferCPU));
    CopyMemory(boxGeometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    boxGeometry->VertexBufferGPU = ZNUtils::CreateDefaultBuffer(graphicDevice.Get(),
        commandList.Get(), vertices.data(), vbByteSize, boxGeometry->VertexBufferUploader);

    boxGeometry->IndexBufferGPU = ZNUtils::CreateDefaultBuffer(graphicDevice.Get(),
        commandList.Get(), indices.data(), ibByteSize, boxGeometry->IndexBufferUploader);

    boxGeometry->VertexByteStride = sizeof(Vertex);
    boxGeometry->VertexBufferByteSize = vbByteSize;
    boxGeometry->IndexFormat = DXGI_FORMAT_R16_UINT;
    boxGeometry->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;
    boxGeometry->DrawArgs["box"] = submesh;
}

void TestApp::BuildPSO()
{
    D3D12_RASTERIZER_DESC rasDesc{};
    rasDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasDesc.FrontCounterClockwise = false;
    rasDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasDesc.DepthClipEnable = true;
    rasDesc.MultisampleEnable = false;
    rasDesc.AntialiasedLineEnable = false;
    rasDesc.ForcedSampleCount = 0;
    rasDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blendDesc{};
    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.IndependentBlendEnable = false;
    blendDesc.RenderTarget[0].BlendEnable = false;
    blendDesc.RenderTarget[0].LogicOpEnable = false;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_DEPTH_STENCIL_DESC dsDesc{};
    dsDesc.DepthEnable = true;
    dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    dsDesc.StencilEnable = false;
    dsDesc.StencilReadMask = 0;
    dsDesc.StencilWriteMask = 0;
    dsDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
    dsDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() };
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.VS =
    {
        reinterpret_cast<BYTE*>(vsByteCode->GetBufferPointer()),
        vsByteCode->GetBufferSize()
    };
    psoDesc.PS =
    {
        reinterpret_cast<BYTE*>(psByteCode->GetBufferPointer()),
        psByteCode->GetBufferSize()
    };
    psoDesc.RasterizerState = rasDesc;
    psoDesc.BlendState = blendDesc;
    psoDesc.DepthStencilState = dsDesc;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = backBufferFormat;
    psoDesc.SampleDesc.Count = _4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = depthStencilFormat;
    ThrowIfFailed(graphicDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PSO)));
}
*/