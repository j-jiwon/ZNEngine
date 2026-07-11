#pragma once
#include "../RenderGraph.h"
#include <d3d12.h>
#include <string>
#include <vector>

namespace ZNFramework {

// Base class for full-screen post-process passes: reads one or more RGResources
// (bound as SRVs) and writes into another RGResource (bound as the RTV), e.g. tone
// mapping the HDR SceneColor + Bloom targets into the LDR back buffer. Handles the
// shared input/output resource transitions; derived passes implement Draw() for the
// actual full-screen quad.
class PostProcessPass : public RenderPass {
public:
    PostProcessPass(std::string name, std::vector<std::string> inputResources, std::string outputResource)
        : RenderPass(std::move(name))
        , inputResourceNames(std::move(inputResources))
        , outputResourceName(std::move(outputResource))
    {}

    void Execute(ID3D12GraphicsCommandList* cmd, RenderGraph& rg) override {
        std::vector<RGResource*> inputs;
        inputs.reserve(inputResourceNames.size());
        for (const auto& name : inputResourceNames) {
            RGResource* res = rg.GetResource(name);
            rg.Transition(cmd, res, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            inputs.push_back(res);
        }

        RGResource* output = rg.GetResource(outputResourceName);
        rg.Transition(cmd, output, D3D12_RESOURCE_STATE_RENDER_TARGET);

        Draw(cmd, rg, inputs, output);
    }

protected:
    virtual void Draw(ID3D12GraphicsCommandList* cmd, RenderGraph& rg,
                      const std::vector<RGResource*>& inputs, RGResource* output) = 0;

private:
    std::vector<std::string> inputResourceNames;
    std::string outputResourceName;
};

} // namespace ZNFramework
