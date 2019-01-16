#include "drape/pointers.hpp"
#include "drape/vertex_array_buffer.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_gpu_buffer_impl.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <cstring>
#include <utility>

namespace dp
{
namespace vulkan
{
class VulkanVertexArrayBufferImpl : public VertexArrayBufferImpl
{
public:
  explicit VulkanVertexArrayBufferImpl(ref_ptr<VertexArrayBuffer> buffer)
    : m_vertexArrayBuffer(buffer)
  {}
  
  bool Build(ref_ptr<GpuProgram> program) override
  {
    UNUSED_VALUE(program);
    return true;
  }
  
  bool Bind() override { return true; }
  void Unbind() override {}
  void BindBuffers(dp::BuffersMap const & buffers) const override {}
  
  void RenderRange(ref_ptr<GraphicsContext> context, bool drawAsLine,
                   IndicesRange const & range) override
  {
    CHECK(false, ());

//    ref_ptr<dp::metal::MetalBaseContext> metalContext = context;
//    if (!metalContext->HasAppliedPipelineState())
//      return;

//    id<MTLRenderCommandEncoder> encoder = metalContext->GetCommandEncoder();
//
//    uint32_t bufferIndex = 0;
//    for (auto & buffer : m_vertexArrayBuffer->m_staticBuffers)
//    {
//      ref_ptr<MetalGpuBufferImpl> b = buffer.second->GetBuffer();
//      [encoder setVertexBuffer:b->GetMetalBuffer() offset:0 atIndex:bufferIndex];
//      bufferIndex++;
//    }
//    for (auto & buffer : m_vertexArrayBuffer->m_dynamicBuffers)
//    {
//      ref_ptr<MetalGpuBufferImpl> b = buffer.second->GetBuffer();
//      [encoder setVertexBuffer:b->GetMetalBuffer() offset:0 atIndex:bufferIndex];
//      bufferIndex++;
//    }
//
//    ref_ptr<MetalGpuBufferImpl> ib = m_vertexArrayBuffer->m_indexBuffer->GetBuffer();
//    auto const isSupported32bit = dp::IndexStorage::IsSupported32bit();
//    auto const indexType = isSupported32bit ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16;
//    auto const indexSize = isSupported32bit ? sizeof(unsigned int) : sizeof(unsigned short);
//
//    [encoder drawIndexedPrimitives:(drawAsLine ? MTLPrimitiveTypeLine : MTLPrimitiveTypeTriangle)
//                        indexCount:range.m_idxCount
//                         indexType:indexType
//                       indexBuffer:ib->GetMetalBuffer()
//                 indexBufferOffset:range.m_idxStart * indexSize];
  }
  
private:
  ref_ptr<VertexArrayBuffer> m_vertexArrayBuffer;
};
}  // namespace vulkan
  
drape_ptr<VertexArrayBufferImpl> VertexArrayBuffer::CreateImplForVulkan(ref_ptr<VertexArrayBuffer> buffer)
{
  return make_unique_dp<vulkan::VulkanVertexArrayBufferImpl>(buffer);
}
}  // namespace dp
