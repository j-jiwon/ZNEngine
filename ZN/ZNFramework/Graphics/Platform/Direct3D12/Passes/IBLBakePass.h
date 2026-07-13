#pragma once
#include "../RenderGraph.h"
#include "../IBLBaker.h"
#include "../CommandQueue.h"

namespace ZNFramework {

// Bakes the BRDF LUT (always, once — self-guarded internally) and updates the
// irradiance/prefiltered env maps every frame from whichever env cubemap the active
// scene registered (see CommandQueue::SetEnvCubemapSRV/ClearEnvCubemapSRV, driven by
// ZNScene::ApplyEnvCubemap() on scene switch). IBLBaker::UpdateEnvironment() itself
// only actually re-bakes when the source changes (or falls back to black when the
// active scene has none), so this is cheap to call unconditionally every frame.
// Registered right after the cubemap-capture passes and before GBufferPass so
// DeferredLightingRenderPass sees fresh data the same frame, exactly like
// CubeCapturePass -> DeferredLightingRenderPass today.
class IBLBakePass : public RenderPass {
public:
    IBLBakePass(IBLBaker* iblBaker, CommandQueue* queue)
        : RenderPass("IBLBake"), iblBaker(iblBaker), queue(queue)
    {}

    void Execute(ID3D12GraphicsCommandList* cmd, RenderGraph& rg) override {
        if (!iblBaker || !queue) return;

        iblBaker->BakeBRDFLUT(cmd);
        iblBaker->UpdateEnvironment(queue->HasEnvCubemap(), queue->GetEnvCubemapSRV(), cmd);
    }

private:
    IBLBaker*     iblBaker;
    CommandQueue* queue;
};

} // namespace ZNFramework
