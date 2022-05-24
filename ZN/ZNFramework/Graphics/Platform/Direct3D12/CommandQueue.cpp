#include "CommandQueue.h"
#include "SwapChain.h"

using namespace ZNFramework;

CommandQueue::CommandQueue(GraphicsDevice* device, ID3D12CommandQueue* queue)
	: queue(queue)
	, device(device)
{
}

ZNSwapChain* CommandQueue::CreateSwapChain(const ZNWindow* window)
{
	return new SwapChain(device, this, window);
}
