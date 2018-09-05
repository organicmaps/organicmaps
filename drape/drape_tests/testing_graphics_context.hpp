#pragma once

#include "drape/graphics_context.hpp"

// Testing context simulates OpenGLES2 API version.
class TestingGraphicsContext : public dp::GraphicsContext
{
public:
  void Present() override {}
  void MakeCurrent() override {}
  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer) override {}
  void ApplyFramebuffer(std::string const & framebufferLabel) override {}

  void Init(dp::ApiVersion apiVersion) override {}
  dp::ApiVersion GetApiVersion() const override { return dp::ApiVersion::OpenGLES2; }
  std::string GetRendererName() const override { return {}; }
  std::string GetRendererVersion() const override { return {}; }

  void SetClearColor(dp::Color const & color) override {}
  void Clear(uint32_t clearBits) override {}
  void Flush() override {}
  void SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) override {}
  void SetDepthTestEnabled(bool enabled) override {}
  void SetDepthTestFunction(dp::TestFunction depthFunction) override {}
  void SetStencilTestEnabled(bool enabled) override {}
  void SetStencilFunction(dp::StencilFace face, dp::TestFunction stencilFunction) override {}
  void SetStencilActions(dp::StencilFace face, dp::StencilAction stencilFailAction,
                         dp::StencilAction depthFailAction, dp::StencilAction passAction) override {}
};
