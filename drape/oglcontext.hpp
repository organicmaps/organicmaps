#pragma once

#include "drape/graphics_context.hpp"

namespace dp
{
class OGLContext: public GraphicsContext
{
public:
  void Init(ApiVersion apiVersion) override;
  void SetClearColor(dp::Color const & color) override;
  void Clear(uint32_t clearBits) override;
  void Flush() override;
  void SetDepthTestEnabled(bool enabled) override;
  void SetDepthTestFunction(TestFunction depthFunction) override;
  void SetStencilTestEnabled(bool enabled) override;
  void SetStencilFunction(StencilFace face, TestFunction stencilFunction) override;
  void SetStencilActions(StencilFace face, StencilAction stencilFailAction, StencilAction depthFailAction,
                         StencilAction passAction) override;
};
}  // namespace dp
