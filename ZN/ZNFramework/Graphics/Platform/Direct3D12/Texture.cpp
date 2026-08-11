#include "Texture.h"
#include "GraphicsDevice.h"
#include "CommandQueue.h"
#include "Graphics/ZNGraphicsContext.h"
#include "DDSTextureLoader12.h"
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>

using namespace ZNFramework;
using namespace DirectX;

namespace
{
    [[noreturn]] void ThrowTextureFailure(const char* stage, const std::string& source, HRESULT hr)
    {
        std::ostringstream message;
        message << "texture " << stage << " failed: path=" << source
            << ", HRESULT=0x" << std::hex << std::uppercase << static_cast<unsigned long>(hr);
        throw std::runtime_error(message.str());
    }
}

namespace ZNFramework::Platform::Direct3D
{
	ZNTexture* CreateTexture()
	{
		return new Texture();
	}
}

void Texture::Init(const std::wstring& path, bool srgb)
{
	CreateTexture(path);
    CreateView(std::filesystem::path(path).string(), srgb);
}

void Texture::InitFromMemory(const void* data, size_t size, bool srgb)
{
	const std::string source = "<embedded texture>";
	const HRESULT hr = ::LoadFromWICMemory(reinterpret_cast<const uint8_t*>(data), size, WIC_FLAGS_NONE, nullptr, image);
	if (FAILED(hr)) ThrowTextureFailure("decode", source, hr);
	UploadToGPU(source);
	CreateView(source, srgb);
}

void Texture::InitSolidColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	const HRESULT hr = image.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 1);
	if (FAILED(hr)) ThrowTextureFailure("solid-color initialization", "<solid-color texture>", hr);
	uint8_t* pixels = image.GetPixels();
	pixels[0] = r; pixels[1] = g; pixels[2] = b; pixels[3] = a;
	UploadToGPU("<solid-color texture>");
	CreateView("<solid-color texture>", false);
}

void Texture::CreateTexture(const std::wstring& path)
{
	std::wstring extension = std::filesystem::path(path).extension();
	HRESULT hr = E_FAIL;

	if (extension == L".dds" || extension == L".DDS")
	{
		hr = ::LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, nullptr, image);
	}
	else if (extension == L".tga" || extension == L".TGA")
	{
		hr = ::LoadFromTGAFile(path.c_str(), nullptr, image);
	}
	else // png, jpg, jpeg, bmp
	{
		hr = ::LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, nullptr, image);
	}

	const std::string source = std::filesystem::path(path).string();
	if (FAILED(hr)) ThrowTextureFailure("decode", source, hr);
	UploadToGPU(source);
}

void Texture::UploadToGPU(const std::string& source)
{
	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    HRESULT hr = ::CreateTexture(device->Device().Get(), image.GetMetadata(), &tex2d);
    if (FAILED(hr)) ThrowTextureFailure("resource creation", source, hr);

    std::vector<D3D12_SUBRESOURCE_DATA> subResources;
    hr = ::PrepareUpload(device->Device().Get(), image.GetImages(), image.GetImageCount(), image.GetMetadata(), subResources);
    if (FAILED(hr)) ThrowTextureFailure("upload preparation", source, hr);

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
        ThrowTextureFailure("upload heap creation", source, hr);

    CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
    // UpdateSubresources returns the number of bytes uploaded (UINT64), 0 on failure — NOT an
    // HRESULT. The old `hr = UpdateSubresources(...); if (FAILED(hr))` truncated that byte count
    // to HRESULT (C4244) and mis-checked it. Check the documented failure sentinel instead.
    const UINT64 uploadedBytes = ::UpdateSubresources(queue->ResourceCommandList(),
        tex2d.Get(),
        textureUploadHeap.Get(),
        0, 0,
        static_cast<unsigned int>(subResources.size()),
        subResources.data());

    if (uploadedBytes == 0)
        ThrowTextureFailure("subresource upload", source, E_FAIL);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        tex2d.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    queue->ResourceCommandList()->ResourceBarrier(1, &barrier);

    queue->FlushResourceQueue();
}

void Texture::CreateView(const std::string& source, bool srgb)
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();
	const HRESULT hr = device->Device()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));
	if (FAILED(hr)) ThrowTextureFailure("SRV descriptor heap creation", source, hr);
	srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = srgb ? DirectX::MakeSRGB(image.GetMetadata().format) : image.GetMetadata().format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	device->Device()->CreateShaderResourceView(tex2d.Get(), &srvDesc, srvHandle);
}
