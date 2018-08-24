#pragma once

#include "drape/graphics_context_factory.hpp"
#include "drape/pointers.hpp"

#import "MetalContext.h"
#import "MetalView.h"

class MetalContextFactory: public dp::GraphicsContextFactory
{
public:
  MetalContextFactory(MetalView * view);
  ~MetalContextFactory() override;
  dp::GraphicsContext * GetDrawContext() override;
  dp::GraphicsContext * GetResourcesUploadContext() override;
  bool IsDrawContextCreated() const override;
  bool IsUploadContextCreated() const override;
  void WaitForInitialization(dp::GraphicsContext * context) override;
  void SetPresentAvailable(bool available) override;
  
private:
  MetalView * m_view;
  drape_ptr<MetalContext> m_context;
};
