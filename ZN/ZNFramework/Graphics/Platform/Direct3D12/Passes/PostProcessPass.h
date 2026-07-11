#pragma once
#include "../RenderGraph.h"
#include <d3d12.h>
#include <string>

namespace ZNFramework {

// Base class for full-screen post-process passes: reads one RGResource (bound as an
// SRV) and writes into another RGResource (bound as the RTV), e.g. tone mapping the
// HDR SceneColor target into the LDR back buffer. Handles the shared input/output
// resource transitions; derived passes implement Draw() for the actual full-screen quad.
class PostProcessPass : public RenderPass {
public:
    PostProcessPass(std::string name, std::string inputResource, std::string outputResource)
        : RenderPass(std::move(name))
        , inputResourceName(std::move(inputResource))
        , outputResourceName(std::move(outputResource))
    {}

    void Execute(ID3D12GraphicsCommandList* cmd, RenderGraph& rg) override {
        RGResource* input  = rg.GetResource(inputResourceName);
        RGResource* output = rg.GetResource(outputResourceName);

        rg.Transition(cmd, input,  D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        rg.Transition(cmd, output, D3D12_RESOURCE_STATE_RENDER_TARGET);

        Draw(cmd, rg, input, output);
    }

protected:
    virtual void Draw(ID3D12GraphicsCommandList* cmd, RenderGraph& rg,
                      RGResource* input, RGResource* output) = 0;

private:
    std::string inputResourceName;
    std::string outputResourceName;
};

} // namespace ZNFramework
