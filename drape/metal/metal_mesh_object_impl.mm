#import <MetalKit/MetalKit.h>

#include "drape/metal/metal_base_context.hpp"
#include "drape/mesh_object.hpp"
#include "drape/pointers.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <cstring>
#include <sstream>

namespace dp
{
namespace metal
{
namespace
{
MTLPrimitiveType GetPrimitiveType(MeshObject::DrawPrimitive primitive)
{
  switch (primitive)
  {
  case MeshObject::DrawPrimitive::Triangles: return MTLPrimitiveTypeTriangle;
  case MeshObject::DrawPrimitive::TriangleStrip: return MTLPrimitiveTypeTriangleStrip;
  case MeshObject::DrawPrimitive::LineStrip: return MTLPrimitiveTypeLineStrip;
  }
  CHECK(false, ("Unsupported type"));
}
}  // namespace

class MetalMeshObjectImpl : public MeshObjectImpl
{
public:
  MetalMeshObjectImpl(ref_ptr<dp::MeshObject> mesh)
    : m_mesh(std::move(mesh))
  {}
  
  void Build(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program) override
  {
    ref_ptr<dp::metal::MetalBaseContext> metalContext = context;
    id<MTLDevice> device = metalContext->GetMetalDevice();
    
    m_geometryBuffers.resize(m_mesh->m_buffers.size());
    for (size_t i = 0; i < m_mesh->m_buffers.size(); i++)
    {
      if (m_mesh->m_buffers[i].m_data.empty())
        continue;
      
      auto const sizeInBytes = m_mesh->m_buffers[i].m_data.size() * sizeof(m_mesh->m_buffers[i].m_data[0]);
      m_geometryBuffers[i] = [device newBufferWithBytes:m_mesh->m_buffers[i].m_data.data()
                                                 length:sizeInBytes
                                                options:MTLResourceCPUCacheModeWriteCombined];
      std::ostringstream ss;
      for (size_t j = 0; j < m_mesh->m_buffers[i].m_attributes.size(); j++)
      {
        ss << m_mesh->m_buffers[i].m_attributes[j].m_attributeName;
        if (j + 1 < m_mesh->m_buffers[i].m_attributes.size())
          ss << "+";
      }
      m_geometryBuffers[i].label = @(ss.str().c_str());
    }
  }
  
  void Reset() override
  {
    m_geometryBuffers.clear();
  }
  
  void UpdateBuffer(uint32_t bufferInd) override
  {
    CHECK_LESS(bufferInd, static_cast<uint32_t>(m_geometryBuffers.size()), ());
    
    auto & buffer = m_mesh->m_buffers[bufferInd];
    CHECK(!buffer.m_data.empty(), ());
    
    uint8_t * bufferPointer = (uint8_t *)[m_geometryBuffers[bufferInd] contents];
    auto const sizeInBytes = buffer.m_data.size() * sizeof(buffer.m_data[0]);
    memcpy(bufferPointer, buffer.m_data.data(), sizeInBytes);
  }
  
  void Bind(ref_ptr<dp::GpuProgram> program) override {}
  
  void Unbind() override {}
  
  void DrawPrimitives(ref_ptr<dp::GraphicsContext> context, uint32_t verticesCount) override
  {
    ref_ptr<dp::metal::MetalBaseContext> metalContext = context;
    if (!metalContext->HasAppliedPipelineState())
      return;
    
    id<MTLRenderCommandEncoder> encoder = metalContext->GetCommandEncoder();
    for (size_t i = 0; i < m_geometryBuffers.size(); i++)
      [encoder setVertexBuffer:m_geometryBuffers[i] offset:0 atIndex:i];
    
    [encoder drawPrimitives:GetPrimitiveType(m_mesh->m_drawPrimitive) vertexStart:0
                vertexCount:verticesCount];
  }
  
private:
  ref_ptr<dp::MeshObject> m_mesh;
  std::vector<id<MTLBuffer>> m_geometryBuffers;
};
}  // namespace metal
  
void MeshObject::InitForMetal()
{
  m_impl = make_unique_dp<metal::MetalMeshObjectImpl>(make_ref(this));
}
}  // namespace dp
