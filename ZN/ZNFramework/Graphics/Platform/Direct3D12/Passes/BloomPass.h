#pragma once
#include "PostProcessPass.h"
#include "../BloomChain.h"
#include "../RenderTexture.h"

namespace ZNFramework {

// Thin RenderGraph wrapper around BloomChain: reads the HDR "SceneColor" resource
// and writes the "Bloom" resource (BloomChain's mip0), which ToneMappingPass then
// reads and adds back onto SceneColor before tone mapping.
class BloomPass : public PostProcessPass {
public:
    BloomPass(RenderTexture* sceneColor, BloomChain* bloomChain)
        : PostProcessPass("Bloom", { "SceneColor" }, "Bloom")
        , sceneColor(sceneColor), bloomChain(bloomChain)
    {}

protected:
    void Draw(ID3D12GraphicsCommandList* cmd, RenderGraph& rg,
             const std::vector<RGResource*>& inputs, RGResource* output) override {
        if (bloomChain)
            bloomChain->Render(sceneColor, cmd);
    }

private:
    RenderTexture* sceneColor;
    BloomChain*    bloomChain;
};

} // namespace ZNFramework
