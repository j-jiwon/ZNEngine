#pragma once
#include "../../ZNCommandList.h"
#include "ZNUtils.h"
#include "GraphicsDevice.h"
#include "Texture.h"
#include "../../../../ZNFramework.h"

namespace ZNFramework
{
    class ZNTexture;
    class CommandList : public ZNCommandList
    {
    public:
        CommandList(GraphicsDevice*, ID3D12CommandAllocator*, ID3D12GraphicsCommandList*, D3D12_COMMAND_LIST_TYPE);
        ~CommandList() noexcept = default;

        void Reset() override;
        void Close() override;

        void SetViewport(int width, int height) override;
        void SetScissorRects(int width, int height) override;

        ID3D12GraphicsCommandList* List() const { return commandList.Get(); }

    private:
        D3D12_COMMAND_LIST_TYPE type;
        ComPtr<ID3D12GraphicsCommandList> commandList;
        ComPtr<ID3D12CommandAllocator> allocator;

        D3D12_VIEWPORT screenViewport;
        D3D12_RECT scissorRect;

        GraphicsDevice* device;
    };
}
