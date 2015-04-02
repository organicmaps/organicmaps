#include "drape/vertex_array_buffer.hpp"
#include "drape/glfunctions.hpp"
#include "drape/glextensions_list.hpp"

#include "base/stl_add.hpp"
#include "base/assert.hpp"

namespace dp
{

VertexArrayBuffer::VertexArrayBuffer(uint32_t indexBufferSize, uint32_t dataBufferSize)
  : m_VAO(0)
  , m_dataBufferSize(dataBufferSize)
  , m_program()
{
  m_indexBuffer.Reset(new IndexBuffer(indexBufferSize));
}

VertexArrayBuffer::~VertexArrayBuffer()
{
  m_indexBuffer.Destroy();
  DeleteRange(m_staticBuffers, MasterPointerDeleter());
  DeleteRange(m_dynamicBuffers, MasterPointerDeleter());

  if (m_VAO != 0)
  {
    /// Build called only when VertexArrayBuffer fulled and transfer to FrontendRenderer
    /// but if user move screen before all geometry readed from MWM we delete VertexArrayBuffer on BackendRenderer
    /// in this case m_VAO will be equal a 0
    /// also m_VAO == 0 will be on device that not support OES_vertex_array_object extension
    GLFunctions::glDeleteVertexArray(m_VAO);
  }
}

void VertexArrayBuffer::Preflush()
{
  /// buffers are ready, so moving them from CPU to GPU
  for(auto & buffer : m_staticBuffers)
    buffer.second->MoveToGPU(GPUBuffer::ElementBuffer);

  for(auto & buffer : m_dynamicBuffers)
    buffer.second->MoveToGPU(GPUBuffer::ElementBuffer);

  ASSERT(!m_indexBuffer.IsNull(), ());
  m_indexBuffer->MoveToGPU(GPUBuffer::IndexBuffer);

  GLFunctions::glBindBuffer(0, gl_const::GLElementArrayBuffer);
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

void VertexArrayBuffer::Render()
{
  RenderRange({ 0, GetIndexBuffer().GetCurrentSize() });
}

void VertexArrayBuffer::RenderRange(IndicesRange const & range)
{
  if (!(m_staticBuffers.empty() && m_dynamicBuffers.empty()) && GetIndexCount() > 0)
  {
    ASSERT(!m_program.IsNull(), ("Somebody not call Build. It's very bad. Very very bad"));
    /// if OES_vertex_array_object is supported than all bindings already saved in VAO
    /// and we need only bind VAO. In Bind method have ASSERT("bind already called")
    if (GLExtensionsList::Instance().IsSupported(GLExtensionsList::VertexArrayObject))
      Bind();
    else
      BindStaticBuffers();

    BindDynamicBuffers();
    GetIndexBuffer().Bind();
    GLFunctions::glDrawElements(range.m_idxCount, range.m_idxStart);
  }
}

void VertexArrayBuffer::Build(RefPointer<GpuProgram> program)
{
  ASSERT(m_VAO == 0 && m_program.IsNull(), ("No-no-no! You can't rebuild VertexArrayBuffer"));
  m_program = program;
  /// if OES_vertex_array_object not supported, than buffers will be bind on each Render call
  if (!GLExtensionsList::Instance().IsSupported(GLExtensionsList::VertexArrayObject))
    return;

  if (m_staticBuffers.empty())
    return;

  m_VAO = GLFunctions::glGenVertexArray();
  Bind();
  BindStaticBuffers();
}

void VertexArrayBuffer::UploadData(BindingInfo const & bindingInfo, void const * data, uint16_t count)
{
  RefPointer<DataBuffer> buffer;
  if (!bindingInfo.IsDynamic())
    buffer = GetOrCreateStaticBuffer(bindingInfo);
  else
    buffer = GetOrCreateDynamicBuffer(bindingInfo);

  buffer->GetBuffer()->UploadData(data, count);
}

RefPointer<DataBuffer> VertexArrayBuffer::GetOrCreateDynamicBuffer(BindingInfo const & bindingInfo)
{
  return GetOrCreateBuffer(bindingInfo, true);
}

RefPointer<DataBuffer> VertexArrayBuffer::GetDynamicBuffer(BindingInfo const & bindingInfo) const
{
  return GetBuffer(bindingInfo, true);
}

RefPointer<DataBuffer> VertexArrayBuffer::GetOrCreateStaticBuffer(BindingInfo const & bindingInfo)
{
  return GetOrCreateBuffer(bindingInfo, false);
}

RefPointer<DataBuffer> VertexArrayBuffer::GetBuffer(BindingInfo const & bindingInfo, bool isDynamic) const
{
  TBuffersMap const * buffers = NULL;
  if (isDynamic)
    buffers = &m_dynamicBuffers;
  else
    buffers = &m_staticBuffers;

  TBuffersMap::const_iterator it = buffers->find(bindingInfo);
  if (it == buffers->end())
    return RefPointer<DataBuffer>();

  return it->second.GetRefPointer();
}

RefPointer<DataBuffer> VertexArrayBuffer::GetOrCreateBuffer(BindingInfo const & bindingInfo, bool isDynamic)
{
  TBuffersMap * buffers = NULL;
  if (isDynamic)
    buffers = &m_dynamicBuffers;
  else
    buffers = &m_staticBuffers;

  TBuffersMap::iterator it = buffers->find(bindingInfo);
  if (it == buffers->end())
  {
    MasterPointer<DataBuffer> & buffer = (*buffers)[bindingInfo];
    buffer.Reset(new DataBuffer(bindingInfo.GetElementSize(), m_dataBufferSize));
    return buffer.GetRefPointer();
  }

  return it->second.GetRefPointer();
}

uint16_t VertexArrayBuffer::GetAvailableIndexCount() const
{
  return GetIndexBuffer().GetAvailableSize();
}

uint16_t VertexArrayBuffer::GetAvailableVertexCount() const
{
  if (m_staticBuffers.empty())
    return m_dataBufferSize;

#ifdef DEBUG
  TBuffersMap::const_iterator it = m_staticBuffers.begin();
  uint16_t prev = it->second->GetBuffer()->GetAvailableSize();
  for (; it != m_staticBuffers.end(); ++it)
    ASSERT(prev == it->second->GetBuffer()->GetAvailableSize(), ());
#endif

  return m_staticBuffers.begin()->second->GetBuffer()->GetAvailableSize();
}

uint16_t VertexArrayBuffer::GetStartIndexValue() const
{
  if (m_staticBuffers.empty())
    return 0;

#ifdef DEBUG
  TBuffersMap::const_iterator it = m_staticBuffers.begin();
  uint16_t prev = it->second->GetBuffer()->GetCurrentSize();
  for (; it != m_staticBuffers.end(); ++it)
    ASSERT(prev == it->second->GetBuffer()->GetCurrentSize(), ());
#endif

  return m_staticBuffers.begin()->second->GetBuffer()->GetCurrentSize();
}

uint16_t VertexArrayBuffer::GetDynamicBufferOffset(BindingInfo const & bindingInfo)
{
  return GetOrCreateDynamicBuffer(bindingInfo)->GetBuffer()->GetCurrentSize();
}

uint16_t VertexArrayBuffer::GetIndexCount() const
{
  return GetIndexBuffer().GetCurrentSize();
}

bool VertexArrayBuffer::IsFilled() const
{
  return GetAvailableIndexCount() < 3 || GetAvailableVertexCount() < 3;
}

void VertexArrayBuffer::UploadIndexes(uint16_t const * data, uint16_t count)
{
  ASSERT(count <= GetIndexBuffer().GetAvailableSize(), ());
  GetIndexBuffer().UploadData(data, count);
}

void VertexArrayBuffer::ApplyMutation(RefPointer<IndexBufferMutator> indexMutator,
                                      RefPointer<AttributeBufferMutator> attrMutator)
{
  if (!indexMutator.IsNull())
  {
    ASSERT(!m_indexBuffer.IsNull(), ());
    m_indexBuffer->UpdateData(indexMutator->GetIndexes(), indexMutator->GetIndexCount());
  }

  if (attrMutator.IsNull())
    return;

  typedef AttributeBufferMutator::TMutateData TMutateData;
  typedef AttributeBufferMutator::TMutateNodes TMutateNodes;
  TMutateData const & data = attrMutator->GetMutateData();
  for (TMutateData::const_iterator it = data.begin(); it != data.end(); ++it)
  {
    RefPointer<DataBuffer> buffer = GetDynamicBuffer(it->first);
    ASSERT(!buffer.IsNull(), ());
    DataBufferMapper mapper(buffer);
    TMutateNodes const & nodes = it->second;

    for (size_t i = 0; i < nodes.size(); ++i)
    {
      MutateNode const & node = nodes[i];
      ASSERT_GREATER(node.m_region.m_count, 0, ());
      mapper.UpdateData(node.m_data.GetRaw(), node.m_region.m_offset, node.m_region.m_count);
    }
  }
}

void VertexArrayBuffer::Bind() const
{
  ASSERT(m_VAO != 0, ("You need to call Build method before bind it and render"));
  GLFunctions::glBindVertexArray(m_VAO);
}

void VertexArrayBuffer::BindStaticBuffers() const
{
  BindBuffers(m_staticBuffers);
}

void VertexArrayBuffer::BindDynamicBuffers() const
{
  BindBuffers(m_dynamicBuffers);
}

void VertexArrayBuffer::BindBuffers(TBuffersMap const & buffers) const
{
  TBuffersMap::const_iterator it = buffers.begin();
  for (; it != buffers.end(); ++it)
  {
    BindingInfo const & binding = it->first;
    RefPointer<DataBuffer> buffer = it->second.GetRefPointer();
    buffer->GetBuffer()->Bind();

    for (uint16_t i = 0; i < binding.GetCount(); ++i)
    {
      BindingDecl const & decl = binding.GetBindingDecl(i);
      int8_t attributeLocation = m_program->GetAttributeLocation(decl.m_attributeName);
      assert(attributeLocation != -1);
      GLFunctions::glEnableVertexAttribute(attributeLocation);
      GLFunctions::glVertexAttributePointer(attributeLocation,
                                            decl.m_componentCount,
                                            decl.m_componentType,
                                            false,
                                            decl.m_stride,
                                            decl.m_offset);
    }
  }
}

DataBufferBase & VertexArrayBuffer::GetIndexBuffer()
{
  ASSERT(!m_indexBuffer.IsNull(), ());
  return *(m_indexBuffer->GetBuffer().GetRaw());
}

DataBufferBase const & VertexArrayBuffer::GetIndexBuffer() const
{
  ASSERT(!m_indexBuffer.IsNull(), ());
  return *(m_indexBuffer->GetBuffer().GetRaw());
}

} // namespace dp
