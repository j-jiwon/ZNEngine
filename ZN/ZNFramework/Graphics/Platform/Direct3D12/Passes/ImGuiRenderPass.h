#pragma once
#include "../RenderGraph.h"
#include <d3d12.h>
#include <functional>

namespace ZNFramework {

class ImGuiRenderPass : public RenderPass {
public:
    // heapPtr: pointer to the CommandQueue's imguiSrvHeap member,
    // so that late binding via SetImGuiDescriptorHeap() is picked up automatically.
    ImGuiRenderPass(ID3D12DescriptorHeap** heapPtr, std::function<void()> cb)
        : RenderPass("ImGui"), heapPtr(heapPtr), renderCb(std::move(cb)) {}

    void Execute(ID3D12GraphicsCommandList* cmd, RenderGraph& rg) override {
        if (!heapPtr || !*heapPtr || !renderCb) return;
        cmd->SetDescriptorHeaps(1, heapPtr);
        renderCb();
    }

private:
    ID3D12DescriptorHeap** heapPtr;
    std::function<void()>  renderCb;
};

} // namespace ZNFramework
