#pragma once
#import <MetalKit/MetalKit.h>

#include "drape/graphics_context.hpp"
#include "drape/gpu_program.hpp"
#include "drape/metal/metal_states.hpp"
#include "drape/pointers.hpp"

#include <cstdint>

namespace dp
{
namespace metal
{
class MetalBaseContext : public dp::GraphicsContext
{
public:
  MetalBaseContext(id<MTLDevice> device, id<MTLTexture> depthStencilTexture);
  
  void Present() override;
  void MakeCurrent() override {}
  void DoneCurrent() override {}
  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer) override;
  void ApplyFramebuffer(std::string const & framebufferLabel) override;
  void Init(ApiVersion apiVersion) override;
  ApiVersion GetApiVersion() const override;
  std::string GetRendererName() const override;
  std::string GetRendererVersion() const override;
  
  void SetClearColor(Color const & color) override;
  void Clear(uint32_t clearBits) override;
  void Flush() override {}
  void SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) override;
  void SetDepthTestEnabled(bool enabled) override;
  void SetDepthTestFunction(TestFunction depthFunction) override;
  void SetStencilTestEnabled(bool enabled) override;
  void SetStencilFunction(StencilFace face, TestFunction stencilFunction) override;
  void SetStencilActions(StencilFace face, StencilAction stencilFailAction,
                         StencilAction depthFailAction, StencilAction passAction) override;
  
  id<MTLDevice> GetMetalDevice() const;
  id<MTLRenderCommandEncoder> GetCommandEncoder() const;
  id<MTLDepthStencilState> GetDepthStencilState();
  id<MTLRenderPipelineState> GetPipelineState(ref_ptr<GpuProgram> program, bool blendingEnabled);
  
protected:
  void SetFrameDrawable(id<CAMetalDrawable> drawable);
  bool HasFrameDrawable() const;
  void FinishCurrentEncoding();

  id<MTLDevice> m_device;
  id<MTLTexture> m_depthStencilTexture;
  MTLRenderPassDescriptor * m_renderPassDescriptor;
  id<MTLCommandQueue> m_commandQueue;
  ref_ptr<dp::BaseFramebuffer> m_currentFramebuffer;
  MetalStates::DepthStencilKey m_currentDepthStencilKey;
  MetalStates m_metalStates;
  
  // These objects are recreated each frame. They MUST NOT be stored anywhere.
  id<CAMetalDrawable> m_frameDrawable;
  id<MTLCommandBuffer> m_frameCommandBuffer;
  id<MTLRenderCommandEncoder> m_currentCommandEncoder;
};
}  // namespace metal
}  // namespace dp
