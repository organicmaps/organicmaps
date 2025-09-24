#import <MetalKit/MetalKit.h>

#include "drape/instancing.hpp"
#include "drape/metal/metal_base_context.hpp"

#include "base/assert.hpp"

namespace dp
{
#if defined(OMIM_METAL_AVAILABLE)
void DrawInstancedTriangleStripMetal(ref_ptr<dp::GraphicsContext> context, uint32_t instanceCount,
                                     uint32_t verticesCount)
{
  ref_ptr<dp::metal::MetalBaseContext> metalContext = context;
  if (!metalContext->HasAppliedPipelineState())
    return;

  id<MTLRenderCommandEncoder> encoder = metalContext->GetCommandEncoder();
  CHECK(encoder != nil, ("Metal render command encoder is required"));

  [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
              vertexStart:0
              vertexCount:verticesCount
            instanceCount:instanceCount];
}
#endif  // OMIM_METAL_AVAILABLE
}  // namespace dp
