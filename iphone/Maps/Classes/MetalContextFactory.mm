#import "MetalContextFactory.h"

#include "base/assert.hpp"

namespace
{
class DrawMetalContext : public dp::metal::MetalBaseContext
{
public:
  DrawMetalContext(CAMetalLayer * metalLayer, id<MTLTexture> depthStencilTexture)
    : dp::metal::MetalBaseContext(metalLayer.device, depthStencilTexture)
    , m_metalLayer(metalLayer)
  {}
  
  bool Validate() override
  {
    if (HasFrameDrawable())
      return true;
    
    id<CAMetalDrawable> drawable = [m_metalLayer nextDrawable];
    SetFrameDrawable(drawable);
    return drawable != nil;
  }
  
private:
  CAMetalLayer * m_metalLayer;
};
  
class UploadMetalContext : public dp::metal::MetalBaseContext
{
public:
  explicit UploadMetalContext(id<MTLDevice> device)
    : dp::metal::MetalBaseContext(device, nil)
  {}
  
  void Present() override {}
  void MakeCurrent() override {}
  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer) override {}
  void Init(dp::ApiVersion apiVersion) override
  {
    CHECK_EQUAL(apiVersion, dp::ApiVersion::Metal, ());
  }
  bool Validate() override { return true; }
  
  void SetClearColor(dp::Color const & color) override {}
  void Clear(uint32_t clearBits) override {}
  void Flush() override {}
  void SetDepthTestEnabled(bool enabled) override {}
  void SetDepthTestFunction(dp::TestFunction depthFunction) override {}
  void SetStencilTestEnabled(bool enabled) override {}
  void SetStencilFunction(dp::StencilFace face, dp::TestFunction stencilFunction) override {}
  void SetStencilActions(dp::StencilFace face, dp::StencilAction stencilFailAction,
                         dp::StencilAction depthFailAction, dp::StencilAction passAction) override {}
};
}  // namespace

MetalContextFactory::MetalContextFactory(MetalView * metalView)
{
  CAMetalLayer * metalLayer = (CAMetalLayer *)metalView.layer;
  m_drawContext = make_unique_dp<DrawMetalContext>(metalLayer, metalView.depthStencilTexture);
  m_uploadContext = make_unique_dp<UploadMetalContext>(m_drawContext->GetMetalDevice());
}

dp::GraphicsContext * MetalContextFactory::GetDrawContext()
{
  return m_drawContext.get();
}

dp::GraphicsContext * MetalContextFactory::GetResourcesUploadContext()
{
  return m_uploadContext.get();
}

void MetalContextFactory::SetPresentAvailable(bool available)
{
  m_drawContext->SetPresentAvailable(available);
}
