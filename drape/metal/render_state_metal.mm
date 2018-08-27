#import <MetalKit/MetalKit.h>

#include "drape/metal/metal_base_context.hpp"
#include "drape/metal/metal_gpu_program.hpp"
#include "drape/pointers.hpp"

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
  [metalContext->GetCommandEncoder() setRenderPipelineState:state];
}
}  // namespace dp
