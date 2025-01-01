#include "Texture.h"
#include "GraphicsDevice.h"
#include "CommandQueue.h"
#include "Graphics/ZNGraphicsContext.h"
#include "DDSTextureLoader12.h"

using namespace ZNFramework;
using namespace DirectX;

namespace ZNFramework::Platform::Direct3D
{
	ZNTexture* CreateTexture()
	{
		return new Texture();
	}
}

void Texture::Init(const std::wstring& path)
{
	CreateTexture(path);
    CreateView();
}

void Texture::CreateTexture(const std::wstring& path)
{
	std::wstring extension = std::filesystem::path(path).extension();

	if (extension == L".dds" || extension == L".DDS")
	{
		::LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, nullptr, image);
	}
	else if (extension == L".tga" || extension == L".TGA")
	{
		::LoadFromTGAFile(path.c_str(), nullptr, image);
	}
	else // png, jpg, jpeg, bmp
	{
		::LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, nullptr, image);
	}

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    // 이미지 크기와 포맷을 가져옵니다.
    UINT width = static_cast<UINT>(image.GetMetadata().width);
    UINT height = static_cast<UINT>(image.GetMetadata().height);
    DXGI_FORMAT format = image.GetMetadata().format;

    // D3D12 리소스 생성
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = format;	// ex) DXGI_FORMAT_R8G8B8A8_UNORM, etc...
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    D3D12_HEAP_PROPERTIES texHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    HRESULT hr = device->Device()->CreateCommittedResource(
        &texHeap,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&tex2d));

    if (FAILED(hr))
    {
        assert(nullptr);
        return;
    }

    std::vector<D3D12_SUBRESOURCE_DATA> subResources;
    D3D12_SUBRESOURCE_DATA subResData = {};
    subResData.pData = image.GetImages()->pixels;
    subResData.RowPitch = static_cast<LONG_PTR>(width * 4);  // assuming 4 bytes per pixel (RGBA)
    subResData.SlicePitch = subResData.RowPitch * height;
    subResources.push_back(subResData);

    const uint64 bufferSize = ::GetRequiredIntermediateSize(tex2d.Get(), 0, static_cast<uint32>(subResources.size()));

    D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    ComPtr<ID3D12Resource> textureUploadHeap;
    hr = device->Device()->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(textureUploadHeap.GetAddressOf()));

    if (FAILED(hr))
    {
        assert(nullptr);
        return;
    }

    CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
    hr = ::UpdateSubresources(queue->ResourceCommandList(),
        tex2d.Get(),
        textureUploadHeap.Get(),
        0, 0,
        static_cast<unsigned int>(subResources.size()),
        subResources.data());

    if (FAILED(hr))
    {
        assert(nullptr);
        return;
    }

    queue->FlushResourceQueue();
}

void Texture::CreateView()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	device->Device()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));
	srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = image.GetMetadata().format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	device->Device()->CreateShaderResourceView(tex2d.Get(), &srvDesc, srvHandle);
}
