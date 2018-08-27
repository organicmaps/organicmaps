#include "drape/metal/metal_base_context.hpp"
#include "drape/metal/metal_gpu_program.hpp"
#include "drape/metal/metal_texture.hpp"

#include "drape/framebuffer.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include <utility>

namespace dp
{
namespace metal
{
MetalBaseContext::MetalBaseContext(id<MTLDevice> device, id<MTLTexture> depthStencilTexture)
  : m_device(device)
  , m_depthStencilTexture(depthStencilTexture)
{
  m_renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
  m_renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
  m_renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
  m_renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
  m_renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
  m_renderPassDescriptor.depthAttachment.clearDepth = 1.0;
  m_renderPassDescriptor.stencilAttachment.loadAction = MTLLoadActionClear;
  m_renderPassDescriptor.stencilAttachment.storeAction = MTLStoreActionStore;
  m_renderPassDescriptor.stencilAttachment.clearStencil = 0;
}

void MetalBaseContext::Init(dp::ApiVersion apiVersion)
{
  CHECK(apiVersion == dp::ApiVersion::Metal, ());
  m_commandQueue = [m_device newCommandQueue];
}

ApiVersion MetalBaseContext::GetApiVersion() const
{
  return dp::ApiVersion::Metal;
}
  
std::string MetalBaseContext::GetRendererName() const
{
  return std::string([m_device.name UTF8String]);
}

std::string MetalBaseContext::GetRendererVersion() const
{
  static std::vector<std::pair<MTLFeatureSet, std::string>> features;
  if (features.empty())
  {
    features.reserve(12);
    features.emplace_back(MTLFeatureSet_iOS_GPUFamily1_v1, "iOS_GPUFamily1_v1");
    features.emplace_back(MTLFeatureSet_iOS_GPUFamily2_v1, "iOS_GPUFamily2_v1");
    features.emplace_back(MTLFeatureSet_iOS_GPUFamily1_v2, "iOS_GPUFamily1_v2");
    features.emplace_back(MTLFeatureSet_iOS_GPUFamily2_v2, "iOS_GPUFamily2_v2");
    features.emplace_back(MTLFeatureSet_iOS_GPUFamily3_v1, "iOS_GPUFamily3_v1");
    if (@available(iOS 10.0, *))
    {
      features.emplace_back(MTLFeatureSet_iOS_GPUFamily1_v3, "iOS_GPUFamily1_v3");
      features.emplace_back(MTLFeatureSet_iOS_GPUFamily2_v3, "iOS_GPUFamily2_v3");
      features.emplace_back(MTLFeatureSet_iOS_GPUFamily3_v2, "iOS_GPUFamily3_v2");
      features.emplace_back(MTLFeatureSet_iOS_GPUFamily4_v1, "iOS_GPUFamily4_v1");
    }
    if (@available(iOS 11.0, *))
    {
      features.emplace_back(MTLFeatureSet_iOS_GPUFamily1_v4, "iOS_GPUFamily1_v4");
      features.emplace_back(MTLFeatureSet_iOS_GPUFamily2_v4, "iOS_GPUFamily2_v4");
      features.emplace_back(MTLFeatureSet_iOS_GPUFamily3_v3, "iOS_GPUFamily3_v3");
    }
    std::sort(features.begin(), features.end(), [](auto const & s1, auto const & s2)
    {
      return s1.first > s2.first;
    });
  }
  
  for (auto featureSet : features)
  {
    if ([m_device supportsFeatureSet:featureSet.first])
      return featureSet.second;
  }
  return "Unknown";
}

void MetalBaseContext::SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer)
{
  FinishCurrentEncoding();
  m_currentFramebuffer = framebuffer;
}

void MetalBaseContext::ApplyFramebuffer(std::string const & framebufferLabel)
{
  // Initialize frame command buffer if there is no one.
  if (!m_frameCommandBuffer)
  {
    m_frameCommandBuffer = [m_commandQueue commandBuffer];
    m_frameCommandBuffer.label = @"Frame command buffer";
  }
  
  if (!m_currentFramebuffer)
  {
    // Use default(system) framebuffer and depth-stencil.
    m_renderPassDescriptor.colorAttachments[0].texture = m_frameDrawable.texture;
    m_renderPassDescriptor.depthAttachment.texture = m_depthStencilTexture;
    m_renderPassDescriptor.stencilAttachment.texture = nil;
  }
  else
  {
    ref_ptr<Framebuffer> framebuffer = m_currentFramebuffer;
    
    ASSERT(dynamic_cast<MetalTexture *>(framebuffer->GetTexture()->GetHardwareTexture().get()) != nullptr, ());
    ref_ptr<MetalTexture> colorAttachment = framebuffer->GetTexture()->GetHardwareTexture();
    m_renderPassDescriptor.colorAttachments[0].texture = colorAttachment->GetTexture();
    
    auto const depthStencilRef = framebuffer->GetDepthStencilRef();
    if (depthStencilRef != nullptr)
    {
      ASSERT(dynamic_cast<MetalTexture *>(depthStencilRef->GetTexture()->GetHardwareTexture().get()) != nullptr, ());
      ref_ptr<MetalTexture> depthStencilAttachment = depthStencilRef->GetTexture()->GetHardwareTexture();
      m_renderPassDescriptor.depthAttachment.texture = depthStencilAttachment->GetTexture();
      m_renderPassDescriptor.stencilAttachment.texture = depthStencilAttachment->GetTexture();
    }
    else
    {
      m_renderPassDescriptor.depthAttachment.texture = nil;
      m_renderPassDescriptor.stencilAttachment.texture = nil;
    }
  }
  
  CHECK(m_currentCommandEncoder == nil, ("Current command encoder was not finished."));
  m_currentCommandEncoder = [m_frameCommandBuffer renderCommandEncoderWithDescriptor:m_renderPassDescriptor];
  [m_currentCommandEncoder pushDebugGroup:@(framebufferLabel.c_str())];
  
  // Default rendering options.
  [m_currentCommandEncoder setFrontFacingWinding:MTLWindingClockwise];
  [m_currentCommandEncoder setCullMode:MTLCullModeBack];
  [m_currentCommandEncoder setStencilReferenceValue:1];
}

void MetalBaseContext::SetClearColor(dp::Color const & color)
{
  m_renderPassDescriptor.colorAttachments[0].clearColor =
    MTLClearColorMake(color.GetRedF(), color.GetGreenF(), color.GetBlueF(), color.GetAlphaF());
}
  
void MetalBaseContext::Clear(uint32_t clearBits)
{
  if (m_currentCommandEncoder != nil)
  {
    // Encoder has already been created. Here we has to draw fullscreen quad for clearing.
    // TODO(@rokuz,@darina)
  }
  else
  {
    if (clearBits & ClearBits::ColorBit)
      m_renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    else
      m_renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionLoad;
    
    if (clearBits & ClearBits::DepthBit)
      m_renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
    else
      m_renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionLoad;
    
    if (clearBits & ClearBits::StencilBit)
      m_renderPassDescriptor.stencilAttachment.loadAction = MTLLoadActionClear;
    else
      m_renderPassDescriptor.stencilAttachment.loadAction = MTLLoadActionLoad;
  }
}
  
void MetalBaseContext::SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
  id<MTLRenderCommandEncoder> encoder = GetCommandEncoder();
  [encoder setViewport:(MTLViewport){ static_cast<double>(x), static_cast<double>(y),
                                      static_cast<double>(w), static_cast<double>(h),
                                      -1.0, 1.0 }];
  [encoder setScissorRect:(MTLScissorRect){ x, y, w, h }];
}

void MetalBaseContext::SetDepthTestEnabled(bool enabled)
{
  m_currentDepthStencilKey.m_depthEnabled = enabled;
}

void MetalBaseContext::SetDepthTestFunction(dp::TestFunction depthFunction)
{
  m_currentDepthStencilKey.m_depthFunction = depthFunction;
}

void MetalBaseContext::SetStencilTestEnabled(bool enabled)
{
  m_currentDepthStencilKey.m_stencilEnabled = enabled;
}

void MetalBaseContext::SetStencilFunction(dp::StencilFace face, dp::TestFunction stencilFunction)
{
  m_currentDepthStencilKey.SetStencilFunction(face, stencilFunction);
}

void MetalBaseContext::SetStencilActions(dp::StencilFace face,
                                         dp::StencilAction stencilFailAction,
                                         dp::StencilAction depthFailAction,
                                         dp::StencilAction passAction)
{
  m_currentDepthStencilKey.SetStencilActions(face, stencilFailAction, depthFailAction, passAction);
}
  
id<MTLDevice> MetalBaseContext::GetMetalDevice() const
{
  return m_device;
}
  
id<MTLRenderCommandEncoder> MetalBaseContext::GetCommandEncoder() const
{
  CHECK(m_currentCommandEncoder != nil, ("Probably encoding commands were called before ApplyFramebuffer."));
  return m_currentCommandEncoder;
}
  
id<MTLDepthStencilState> MetalBaseContext::GetDepthStencilState()
{
  return m_metalStates.GetDepthStencilState(m_device, m_currentDepthStencilKey);
}
  
id<MTLRenderPipelineState> MetalBaseContext::GetPipelineState(ref_ptr<GpuProgram> program, bool blendingEnabled)
{
  CHECK(m_currentCommandEncoder != nil, ("Probably encoding commands were called before ApplyFramebuffer."));
  
  id<MTLTexture> colorTexture = m_renderPassDescriptor.colorAttachments[0].texture;
  CHECK(colorTexture != nil, ());
  
  id<MTLTexture> depthTexture = m_renderPassDescriptor.depthAttachment.texture;
  MTLPixelFormat depthStencilFormat = (depthTexture != nil) ? depthTexture.pixelFormat : MTLPixelFormatInvalid;
  
  MetalStates::PipelineKey const key(program, colorTexture.pixelFormat, depthStencilFormat, blendingEnabled);
  return m_metalStates.GetPipelineState(m_device, key);
}

void MetalBaseContext::Present()
{
  FinishCurrentEncoding();
  if (m_frameDrawable)
  {
    [m_frameCommandBuffer presentDrawable:m_frameDrawable];
    [m_frameCommandBuffer commit];
  }
  m_frameDrawable = nil;
  [m_frameCommandBuffer waitUntilCompleted];
  m_frameCommandBuffer = nil;
}
  
void MetalBaseContext::SetFrameDrawable(id<CAMetalDrawable> drawable)
{
  CHECK(drawable != nil, ());
  m_frameDrawable = drawable;
}
  
bool MetalBaseContext::HasFrameDrawable() const
{
  return m_frameDrawable != nil;
}
  
void MetalBaseContext::FinishCurrentEncoding()
{
  [m_currentCommandEncoder popDebugGroup];
  [m_currentCommandEncoder endEncoding];
  m_currentCommandEncoder = nil;
}
}  // namespace metal
}  // namespace dp
