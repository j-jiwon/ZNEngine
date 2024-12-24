#include "CommandList.h"

using namespace ZNFramework;

CommandList::CommandList(GraphicsDevice* device, ID3D12CommandAllocator* allocator, ID3D12GraphicsCommandList* list, D3D12_COMMAND_LIST_TYPE type)
	: type(type)
	, commandList(list)
	, allocator(allocator)
	, device(device)
{
}

void CommandList::Reset()
{
	ThrowIfFailed(commandList->Reset(allocator.Get(), nullptr));
}

void CommandList::Close()
{
	ThrowIfFailed(commandList->Close());
}

void CommandList::SetViewport(int width, int height)
{
	screenViewport.TopLeftX = 0;
	screenViewport.TopLeftY = 0;
	screenViewport.Width = static_cast<float>(width);
	screenViewport.Height = static_cast<float>(height);
	screenViewport.MinDepth = 0.0f;
	screenViewport.MaxDepth = 1.0f;
}

void CommandList::SetScissorRects(int width, int height)
{
	scissorRect = { 0, 0, width, height };
}

