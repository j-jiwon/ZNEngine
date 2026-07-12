#pragma once
#include "../RenderGraph.h"
#include "../IBLBaker.h"
#include "../CommandQueue.h"

namespace ZNFramework {

// Bakes the BRDF LUT (always, once) and the irradiance/prefiltered env maps (once a
// real env cubemap is registered — see CommandQueue::SetEnvCubemapSRV, set by
// ZNScene::AddCubemapCapture/SetEnvCubemapTexture). Both bakes self-guard with an
// internal one-shot flag (IBLBaker::brdfBaked/envBaked), so this pass just calls
// through every frame; registered right after the cubemap-capture passes and before
// GBufferPass so DeferredLightingRenderPass sees fresh data the same frame, exactly
// like CubeCapturePass -> DeferredLightingRenderPass today.
class IBLBakePass : public RenderPass {
public:
    IBLBakePass(IBLBaker* iblBaker, CommandQueue* queue)
        : RenderPass("IBLBake"), iblBaker(iblBaker), queue(queue)
    {}

    void Execute(ID3D12GraphicsCommandList* cmd, RenderGraph& rg) override {
        if (!iblBaker) return;

        iblBaker->BakeBRDFLUT(cmd);

        if (queue && queue->HasEnvCubemap())
            iblBaker->BakeEnvironment(queue->GetEnvCubemapSRV(), cmd);
    }

private:
    IBLBaker*     iblBaker;
    CommandQueue* queue;
};

} // namespace ZNFramework
