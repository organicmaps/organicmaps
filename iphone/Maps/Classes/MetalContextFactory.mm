#import "MetalContextFactory.h"

#include "base/assert.hpp"

namespace
{
class DrawMetalContext : public dp::metal::MetalBaseContext
{
public:
  DrawMetalContext(CAMetalLayer * metalLayer, m2::PointU const & screenSize)
    : dp::metal::MetalBaseContext(metalLayer.device, screenSize, [this] { return [m_metalLayer nextDrawable]; })
    , m_metalLayer(metalLayer)
  {}

  void Resize(int w, int h) override
  {
    m_metalLayer.drawableSize = CGSize{static_cast<float>(w), static_cast<float>(h)};
    ResetFrameDrawable();
    dp::metal::MetalBaseContext::Resize(w, h);
  }

private:
  CAMetalLayer * m_metalLayer;
};

class UploadMetalContext : public dp::metal::MetalBaseContext
{
public:
  explicit UploadMetalContext(id<MTLDevice> device) : dp::metal::MetalBaseContext(device, {}, nullptr) {}

  void Present() override {}
  void MakeCurrent() override {}
  void Resize(int w, int h) override {}
  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer) override {}
  void Init(dp::ApiVersion apiVersion) override { CHECK_EQUAL(apiVersion, dp::ApiVersion::Metal, ()); }

  void SetClearColor(dp::Color const & color) override {}
  void Clear(uint32_t clearBits, uint32_t storeBits) override {}
  void Flush() override {}
  void SetDepthTestEnabled(bool enabled) override {}
  void SetDepthTestFunction(dp::TestFunction depthFunction) override {}
  void SetStencilTestEnabled(bool enabled) override {}
  void SetStencilFunction(dp::StencilFace face, dp::TestFunction stencilFunction) override {}
  void SetStencilActions(dp::StencilFace face, dp::StencilAction stencilFailAction, dp::StencilAction depthFailAction,
                         dp::StencilAction passAction) override
  {}
};
}  // namespace

MetalContextFactory::MetalContextFactory(CAMetalLayer * metalLayer, m2::PointU const & screenSize)
{
  m_drawContext = make_unique_dp<DrawMetalContext>(metalLayer, screenSize);
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
