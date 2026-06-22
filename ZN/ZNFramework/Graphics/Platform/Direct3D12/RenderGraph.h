#pragma once
#include <d3d12.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace ZNFramework {

// Tracks a D3D12 resource and its current resource state across passes
struct RGResource {
    std::string           name;
    ID3D12Resource*       resource = nullptr;
    D3D12_RESOURCE_STATES state    = D3D12_RESOURCE_STATE_COMMON;
};

class RenderGraph;

// Base class for a single render pass
class RenderPass {
public:
    explicit RenderPass(std::string name) : name(std::move(name)) {}
    virtual ~RenderPass() = default;

    virtual void Execute(ID3D12GraphicsCommandList* cmd, RenderGraph& rg) = 0;

    const std::string& GetName()  const { return name; }
    bool               IsEnabled() const { return enabled; }
    void               SetEnabled(bool e) { enabled = e; }

protected:
    std::string name;
    bool        enabled = true;
};

// Ordered pass list + resource state tracking with automatic barrier insertion
class RenderGraph {
public:
    // Register/update an external resource. Calling Import again resets the tracked state.
    RGResource* Import(const std::string& name, ID3D12Resource* res, D3D12_RESOURCE_STATES state);

    // Update only the resource pointer (e.g. swap chain back buffer changes each frame)
    void UpdateResource(const std::string& name, ID3D12Resource* res);

    // Look up a registered resource (returns nullptr if not found)
    RGResource* GetResource(const std::string& name);

    // Insert a barrier if the resource is not already in newState
    void Transition(ID3D12GraphicsCommandList* cmd, RGResource* res, D3D12_RESOURCE_STATES newState);

    void        AddPass(std::unique_ptr<RenderPass> pass);
    RenderPass* GetPass(const std::string& name);

    // Execute all enabled passes in registration order
    void Execute(ID3D12GraphicsCommandList* cmd);

private:
    std::unordered_map<std::string, RGResource> resources;
    std::vector<std::unique_ptr<RenderPass>>    passes;
};

} // namespace ZNFramework
