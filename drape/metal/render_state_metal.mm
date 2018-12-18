#import <MetalKit/MetalKit.h>

#include "drape/metal/metal_base_context.hpp"
#include "drape/metal/metal_gpu_program.hpp"
#include "drape/metal/metal_texture.hpp"
#include "drape/pointers.hpp"
#include "drape/render_state.hpp"

#include "base/assert.hpp"

#include <utility>

namespace dp
{
void ApplyDepthStencilStateForMetal(ref_ptr<GraphicsContext> context)
{
  ref_ptr<dp::metal::MetalBaseContext> metalContext = context;
  id<MTLDepthStencilState> state = metalContext->GetDepthStencilState();
  [metalContext->GetCommandEncoder() setDepthStencilState:state];
}
  
void ApplyPipelineStateForMetal(ref_ptr<GraphicsContext> context, ref_ptr<GpuProgram> program,
                                bool blendingEnabled)
{
  ref_ptr<dp::metal::MetalBaseContext> metalContext = context;
  id<MTLRenderPipelineState> state = metalContext->GetPipelineState(std::move(program), blendingEnabled);
  metalContext->ApplyPipelineState(state);
}
  
void ApplyTexturesForMetal(ref_ptr<GraphicsContext> context, ref_ptr<GpuProgram> program,
                           RenderState const & state)
{
  ref_ptr<dp::metal::MetalBaseContext> metalContext = context;
  ref_ptr<dp::metal::MetalGpuProgram> p = program;
  id<MTLRenderCommandEncoder> encoder = metalContext->GetCommandEncoder();
  for (auto const & texture : state.GetTextures())
  {
    if (texture.second == nullptr)
      continue;
    
    ref_ptr<dp::metal::MetalTexture> t = texture.second->GetHardwareTexture();
    if (t == nullptr)
      continue;
    
    t->SetFilter(state.GetTextureFilter());
    dp::HWTexture::Params const & params = t->GetParams();
    
    // Set texture to the vertex shader.
    auto const & vsBindingInfo = p->GetVertexTextureBindingInfo(texture.first);
    if (vsBindingInfo.m_textureBindingIndex >= 0)
    {
      [encoder setVertexTexture:t->GetTexture() atIndex:vsBindingInfo.m_textureBindingIndex];
      if (vsBindingInfo.m_samplerBindingIndex >= 0)
      {
        id<MTLSamplerState> samplerState = metalContext->GetSamplerState(params.m_filter, params.m_wrapSMode,
                                                                         params.m_wrapTMode);
        [encoder setVertexSamplerState:samplerState atIndex:vsBindingInfo.m_samplerBindingIndex];
      }
    }
    
    // Set texture to the fragment shader.
    auto const & fsBindingInfo = p->GetFragmentTextureBindingInfo(texture.first);
    if (fsBindingInfo.m_textureBindingIndex >= 0)
    {
      [encoder setFragmentTexture:t->GetTexture() atIndex:fsBindingInfo.m_textureBindingIndex];
      if (fsBindingInfo.m_samplerBindingIndex >= 0)
      {
        id<MTLSamplerState> samplerState = metalContext->GetSamplerState(params.m_filter, params.m_wrapSMode,
                                                                         params.m_wrapTMode);
        [encoder setFragmentSamplerState:samplerState atIndex:fsBindingInfo.m_samplerBindingIndex];
      }
    }
  }
}
}  // namespace dp
