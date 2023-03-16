#pragma once
#import <MetalKit/MetalKit.h>

#include "drape/graphics_context_factory.hpp"
#include "drape/metal/metal_base_context.hpp"
#include "drape/pointers.hpp"

class MetalContextFactory: public dp::GraphicsContextFactory
{
public:
  MetalContextFactory(CAMetalLayer * metalLayer, m2::PointU const & screenSize);
  dp::GraphicsContext * GetDrawContext() override;
  dp::GraphicsContext * GetResourcesUploadContext() override;
  bool IsDrawContextCreated() const override { return true; }
  bool IsUploadContextCreated() const override { return true; }
  void WaitForInitialization(dp::GraphicsContext * context) override {}
  void SetPresentAvailable(bool available) override;
  
private:
  drape_ptr<dp::metal::MetalBaseContext> m_drawContext;
  drape_ptr<dp::metal::MetalBaseContext> m_uploadContext;
};
