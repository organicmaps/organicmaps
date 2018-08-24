#pragma once

#include "drape/graphics_context.hpp"

#import "MetalView.h"

class MetalContext: public dp::GraphicsContext
{
public:
  MetalContext(MetalView * view);
  ~MetalContext() override;
  
  void Present() override;
  void MakeCurrent() override;
  void DoneCurrent() override;
  void SetDefaultFramebuffer() override;
  void Resize(int w, int h) override;
  void SetRenderingEnabled(bool enabled) override;
  void SetPresentAvailable(bool available) override;
  bool Validate() override;
  
  void Init(dp::ApiVersion apiVersion) override;
  void SetClearColor(dp::Color const & color) override;
  void Clear(uint32_t clearBits) override;
  void Flush() override;
  void SetDepthTestEnabled(bool enabled) override;
  void SetDepthTestFunction(dp::TestFunction depthFunction) override;
  void SetStencilTestEnabled(bool enabled) override;
  void SetStencilFunction(dp::StencilFace face, dp::TestFunction stencilFunction) override;
  void SetStencilActions(dp::StencilFace face,
                         dp::StencilAction stencilFailAction,
                         dp::StencilAction depthFailAction,
                         dp::StencilAction passAction) override;
  
private:
  MetalView * m_view;
  id<MTLDevice> m_device;
  id<MTLCommandQueue> m_commandQueue;
};
