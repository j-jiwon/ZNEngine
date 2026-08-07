#include "EquirectCubeTexture.h"
#include "GraphicsDevice.h"
#include "CommandQueue.h"
#include "Graphics/ZNGraphicsContext.h"
#include <cmath>
#include <vector>

using namespace ZNFramework;
using namespace DirectX;

namespace {

// D3D cubemap face convention (matches the +X,-X,+Y,-Y,+Z,-Z / up-vector basis already
// used for rendering ZNScene::AddCubemapCapture's 6 faces): face-local UV in [-1,1] maps to
// world direction = normalize(dir + u*right + v*up), where right = normalize(cross(up, dir)).
struct FaceBasis { float dir[3]; float right[3]; float up[3]; };
static const FaceBasis kFaces[6] = {
    { { 1,  0,  0}, { 0, 0, -1}, {0, 1,  0} }, // +X
    { {-1,  0,  0}, { 0, 0,  1}, {0, 1,  0} }, // -X
    { { 0,  1,  0}, { 1, 0,  0}, {0, 0, -1} }, // +Y
    { { 0, -1,  0}, { 1, 0,  0}, {0, 0,  1} }, // -Y
    { { 0,  0,  1}, { 1, 0,  0}, {0, 1,  0} }, // +Z
    { { 0,  0, -1}, {-1, 0,  0}, {0, 1,  0} }, // -Z
};

void SampleBilinear(const uint8_t* src, uint32 srcW, uint32 srcH, uint32 srcRowPitch,
                     float u, float v, uint8_t out[4])
{
    u = u - floorf(u); // wrap horizontally (longitude seam)
    v = (v < 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v); // clamp vertically (poles)

    float fx = u * srcW - 0.5f;
    float fy = v * srcH - 0.5f;
    int x0 = static_cast<int>(floorf(fx));
    int y0 = static_cast<int>(floorf(fy));
    float tx = fx - x0;
    float ty = fy - y0;
    int x1 = x0 + 1, y1 = y0 + 1;

    auto wrapX = [srcW](int x) { x %= static_cast<int>(srcW); if (x < 0) x += srcW; return x; };
    auto clampY = [srcH](int y) {
        int maxY = static_cast<int>(srcH) - 1;
        return (y < 0) ? 0 : (y > maxY ? maxY : y);
    };
    x0 = wrapX(x0); x1 = wrapX(x1);
    y0 = clampY(y0); y1 = clampY(y1);

    auto pixelAt = [&](int x, int y) { return src + static_cast<size_t>(y) * srcRowPitch + static_cast<size_t>(x) * 4; };
    const uint8_t* p00 = pixelAt(x0, y0);
    const uint8_t* p10 = pixelAt(x1, y0);
    const uint8_t* p01 = pixelAt(x0, y1);
    const uint8_t* p11 = pixelAt(x1, y1);
    for (int c = 0; c < 4; ++c)
    {
        float top = p00[c] * (1 - tx) + p10[c] * tx;
        float bot = p01[c] * (1 - tx) + p11[c] * tx;
        out[c] = static_cast<uint8_t>(top * (1 - ty) + bot * ty + 0.5f);
    }
}

} // namespace

void EquirectCubeTexture::Init(const std::wstring& path, uint32 faceSize, bool srgb)
{
    ScratchImage srcLoaded;
    ThrowIfFailed(::LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, nullptr, srcLoaded));

    ScratchImage srcRGBA;
    const Image* srcRaw = srcLoaded.GetImage(0, 0, 0);
    if (srcRaw->format != DXGI_FORMAT_R8G8B8A8_UNORM)
        ThrowIfFailed(::Convert(*srcRaw, DXGI_FORMAT_R8G8B8A8_UNORM, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, srcRGBA));
    else
        srcRGBA = std::move(srcLoaded);

    const Image* srcImg = srcRGBA.GetImage(0, 0, 0);
    const uint8_t* srcPixels = srcImg->pixels;
    const uint32 srcW = static_cast<uint32>(srcImg->width);
    const uint32 srcH = static_cast<uint32>(srcImg->height);
    const uint32 srcRowPitch = static_cast<uint32>(srcImg->rowPitch);

    // Resample onto 6 faces (equirectangular: U = longitude around Y, V = latitude from Y)
    ScratchImage cubeImage;
    ThrowIfFailed(cubeImage.InitializeCube(DXGI_FORMAT_R8G8B8A8_UNORM, faceSize, faceSize, 1, 1));

    const float PI = 3.14159265359f;
    for (uint32 face = 0; face < 6; ++face)
    {
        const Image* dst = cubeImage.GetImage(0, face, 0);
        uint8_t* dstPixels = dst->pixels;
        const uint32 dstRowPitch = static_cast<uint32>(dst->rowPitch);
        const FaceBasis& basis = kFaces[face];

        for (uint32 y = 0; y < faceSize; ++y)
        {
            // v=+1 at top row (y=0) so it lines up with `up`; v=-1 at bottom row.
            float v = 1.0f - (2.0f * (y + 0.5f) / faceSize);
            for (uint32 x = 0; x < faceSize; ++x)
            {
                float u = (2.0f * (x + 0.5f) / faceSize) - 1.0f;

                float dx = basis.dir[0] + u * basis.right[0] + v * basis.up[0];
                float dy = basis.dir[1] + u * basis.right[1] + v * basis.up[1];
                float dz = basis.dir[2] + u * basis.right[2] + v * basis.up[2];
                float len = sqrtf(dx * dx + dy * dy + dz * dz);
                dx /= len; dy /= len; dz /= len;

                float longitude = atan2f(dz, dx);                                  // [-PI, PI]
                float dyClamped = (dy < -1.0f) ? -1.0f : (dy > 1.0f ? 1.0f : dy);
                float latitude  = asinf(dyClamped);                                // [-PI/2, PI/2]
                float eqU = (longitude / (2.0f * PI)) + 0.5f;
                float eqV = 0.5f - (latitude / PI);

                uint8_t rgba[4];
                SampleBilinear(srcPixels, srcW, srcH, srcRowPitch, eqU, eqV, rgba);

                uint8_t* dstPixel = dstPixels + static_cast<size_t>(y) * dstRowPitch + static_cast<size_t>(x) * 4;
                dstPixel[0] = rgba[0]; dstPixel[1] = rgba[1]; dstPixel[2] = rgba[2]; dstPixel[3] = rgba[3];
            }
        }
    }

    // Upload to GPU (same CreateTexture/PrepareUpload/UpdateSubresources pattern as Texture.cpp)
    GraphicsDevice* device = GraphicsContext::GetInstance().GetAs<GraphicsDevice>();

    ThrowIfFailed(::CreateTexture(device->Device().Get(), cubeImage.GetMetadata(), &cubeResource));

    std::vector<D3D12_SUBRESOURCE_DATA> subResources;
    ThrowIfFailed(::PrepareUpload(device->Device().Get(), cubeImage.GetImages(), cubeImage.GetImageCount(),
                                   cubeImage.GetMetadata(), subResources));

    const uint64 bufferSize = ::GetRequiredIntermediateSize(cubeResource.Get(), 0, static_cast<uint32>(subResources.size()));

    D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    ComPtr<ID3D12Resource> uploadHeap;
    ThrowIfFailed(device->Device()->CreateCommittedResource(
        &heapProp, D3D12_HEAP_FLAG_NONE, &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadHeap.GetAddressOf())));

    CommandQueue* queue = GraphicsContext::GetInstance().GetAs<CommandQueue>();
    // UpdateSubresources returns bytes uploaded (UINT64), 0 on failure — not an HRESULT. Wrapping
    // it in ThrowIfFailed truncated the byte count to HRESULT (C4244); check the sentinel instead.
    const UINT64 uploadedBytes = ::UpdateSubresources(queue->ResourceCommandList(), cubeResource.Get(),
        uploadHeap.Get(), 0, 0, static_cast<uint32>(subResources.size()), subResources.data());
    if (uploadedBytes == 0)
        ThrowIfFailed(E_FAIL);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        cubeResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    queue->ResourceCommandList()->ResourceBarrier(1, &barrier);

    queue->FlushResourceQueue(); // synchronous; safe to let uploadHeap go out of scope after this

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device->Device()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));
    srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = srgb ? DirectX::MakeSRGB(cubeImage.GetMetadata().format) : cubeImage.GetMetadata().format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.TextureCube.MipLevels = 1;
    srvDesc.TextureCube.MostDetailedMip = 0;
    device->Device()->CreateShaderResourceView(cubeResource.Get(), &srvDesc, srvHandle);
}
