#pragma once

#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>

namespace dp
{
namespace vulkan
{
class VulkanBaseContext : public dp::GraphicsContext
{
public:
  VulkanBaseContext(m2::PointU const & screenSize);
  
  void Present() override {}
  void MakeCurrent() override {}
  void DoneCurrent() override {}
  bool Validate() override { return true; }
  void Resize(int w, int h) override {}
  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer) override {}
  void ApplyFramebuffer(std::string const & framebufferLabel) override {}
  void Init(ApiVersion apiVersion) override {}
  ApiVersion GetApiVersion() const override { return dp::ApiVersion::Vulkan; }
  std::string GetRendererName() const override { return ""; }
  std::string GetRendererVersion() const override { return ""; }

  void DebugSynchronizeWithCPU() override {}
  void PushDebugLabel(std::string const & label) override {}
  void PopDebugLabel() override {}
  
  void SetClearColor(Color const & color) override {}
  void Clear(uint32_t clearBits, uint32_t storeBits) override {}
  void Flush() override {}
  void SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) override {}
  void SetDepthTestEnabled(bool enabled) override {}
  void SetDepthTestFunction(TestFunction depthFunction) override {}
  void SetStencilTestEnabled(bool enabled) override {}
  void SetStencilFunction(StencilFace face, TestFunction stencilFunction) override {}
  void SetStencilActions(StencilFace face, StencilAction stencilFailAction,
                         StencilAction depthFailAction, StencilAction passAction) override {}
  void SetStencilReferenceValue(uint32_t stencilReferenceValue) override;

protected:
  uint32_t m_stencilReferenceValue = 1;
};
}  // namespace vulkan
}  // namespace dp
