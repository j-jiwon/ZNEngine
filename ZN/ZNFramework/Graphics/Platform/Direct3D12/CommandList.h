#pragma once
#include "../../ZNCommandList.h"
#include "ZNUtils.h"
#include "GraphicsDevice.h"

namespace ZNFramework
{
    class CommandList : public ZNCommandList
    {
    public:
        CommandList(ID3D12CommandAllocator*, ID3D12CommandList*, D3D12_COMMAND_LIST_TYPE);
        ~CommandList() noexcept = default;

        void Reset();

    private:
        D3D12_COMMAND_LIST_TYPE type;
        ComPtr<ID3D12CommandList> list;
        ComPtr<ID3D12CommandAllocator> allocator;
    };
}
