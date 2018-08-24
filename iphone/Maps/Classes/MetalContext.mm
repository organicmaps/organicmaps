#import "MetalContext.h"

MetalContext::MetalContext(MetalView * view)
{
  m_device = view.device;
  m_commandQueue = [m_device newCommandQueue];
}

MetalContext::~MetalContext()
{
  
}

void MetalContext::MakeCurrent()
{
  
}

void MetalContext::DoneCurrent()
{
  
}

void MetalContext::Present()
{
  id<MTLCommandBuffer> commandBuffer = [m_commandQueue commandBuffer];
  commandBuffer.label = @"MyCommand";
  
  MTLRenderPassDescriptor *renderPassDescriptor = m_view.currentRenderPassDescriptor;
  
  if(renderPassDescriptor != nil)
  {
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    
    renderEncoder.label = @"MyRenderEncoder";
    
    [renderEncoder endEncoding];
    
    [commandBuffer presentDrawable:m_view.currentDrawable];
  }
  [commandBuffer commit];
}

void MetalContext::SetDefaultFramebuffer()
{
  
}

void MetalContext::Resize(int w, int h)
{
  
}

void MetalContext::SetRenderingEnabled(bool enabled)
{
  
}

void MetalContext::SetPresentAvailable(bool available)
{
  
}

bool MetalContext::Validate()
{
  return true;
}


void MetalContext::Init(dp::ApiVersion apiVersion)
{
  
}

void MetalContext::SetClearColor(dp::Color const & color)
{
  m_view.clearColor = MTLClearColorMake(color.GetRedF(), color.GetGreen(),
                                        color.GetBlueF(), color.GetAlphaF());
}

void MetalContext::Clear(uint32_t clearBits)
{
}

void MetalContext::Flush()
{
  
}

void MetalContext::SetDepthTestEnabled(bool enabled)
{
  
}

void MetalContext::SetDepthTestFunction(dp::TestFunction depthFunction)
{
  
}

void MetalContext::SetStencilTestEnabled(bool enabled)
{
  
}

void MetalContext::SetStencilFunction(dp::StencilFace face, dp::TestFunction stencilFunction)
{
  
}

void MetalContext::SetStencilActions(dp::StencilFace face,
                                     dp::StencilAction stencilFailAction,
                                     dp::StencilAction depthFailAction,
                                     dp::StencilAction passAction)
{
  
}
