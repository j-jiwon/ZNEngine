#pragma once
#include "PostProcessPass.h"
#include "../SkyboxRenderer.h"
#include "../GBufferManager.h"
#include "../RenderTexture.h"
#include "../CommandQueue.h"
#include "Graphics/ZNGraphicsContext.h"

namespace ZNFramework {

// Main-camera skybox resolve: reads GBuf_DepthCopy (to skip pixels with real geometry)
// and writes into SceneColor, filling background pixels with the active scene's skybox
// (or black, via SkyboxRenderer's own fallback, if none is registered). Registered right
// after DeferredLightingRenderPass and before BloomPass, so a bright skybox still
// contributes to bloom.
class SkyboxPass : public PostProcessPass {
public:
    SkyboxPass(SkyboxRenderer* renderer, GBufferManager* gbufferManager,
              RenderTexture* sceneColor, CommandQueue* queue)
        : PostProcessPass("Skybox", { "GBuf_DepthCopy" }, "SceneColor")
        , renderer(renderer), gbufferManager(gbufferManager), sceneColor(sceneColor), queue(queue)
    {}

protected:
    void Draw(ID3D12GraphicsCommandList* cmd, RenderGraph& rg,
             const std::vector<RGResource*>& inputs, RGResource* output) override {
        if (!renderer || !gbufferManager || !sceneColor || !queue) return;

        ZNCamera* camera = GraphicsContext::GetInstance().GetCamera();
        renderer->DrawResolve(cmd, camera, gbufferManager->GetDepthCopySRV(),
                             queue->HasSkybox(), queue->GetSkyboxSRV(),
                             sceneColor->GetRTV(), sceneColor->GetWidth(), sceneColor->GetHeight());
    }

private:
    SkyboxRenderer* renderer;
    GBufferManager* gbufferManager;
    RenderTexture*  sceneColor;
    CommandQueue*   queue;
};

} // namespace ZNFramework
