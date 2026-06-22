#include "RenderGraph.h"
#include "d3dx12.h"
#include <algorithm>

namespace ZNFramework {

RGResource* RenderGraph::Import(const std::string& name, ID3D12Resource* res, D3D12_RESOURCE_STATES state) {
    auto& r  = resources[name];
    r.name   = name;
    r.resource = res;
    r.state  = state;
    return &r;
}

void RenderGraph::UpdateResource(const std::string& name, ID3D12Resource* res) {
    auto it = resources.find(name);
    if (it != resources.end())
        it->second.resource = res;
}

RGResource* RenderGraph::GetResource(const std::string& name) {
    auto it = resources.find(name);
    return (it != resources.end()) ? &it->second : nullptr;
}

void RenderGraph::Transition(ID3D12GraphicsCommandList* cmd, RGResource* res, D3D12_RESOURCE_STATES newState) {
    if (!res || !res->resource || res->state == newState) return;
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(res->resource, res->state, newState);
    cmd->ResourceBarrier(1, &barrier);
    res->state = newState;
}

void RenderGraph::AddPass(std::unique_ptr<RenderPass> pass) {
    passes.push_back(std::move(pass));
}

RenderPass* RenderGraph::GetPass(const std::string& name) {
    for (auto& p : passes)
        if (p->GetName() == name) return p.get();
    return nullptr;
}

void RenderGraph::Execute(ID3D12GraphicsCommandList* cmd) {
    for (auto& pass : passes)
        if (pass->IsEnabled())
            pass->Execute(cmd, *this);
}

} // namespace ZNFramework
