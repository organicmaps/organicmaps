#include "vertex_array_buffer.hpp"
#include "glfunctions.hpp"
#include "glextensions_list.hpp"

#include "../base/stl_add.hpp"
#include "../base/assert.hpp"

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
  GetRangeDeletor(m_buffers, MasterPointerDeleter())();

  if (m_VAO != 0)
  {
    /// Build called only when VertexArrayBuffer fulled and transfer to FrontendRenderer
    /// but if user move screen before all geometry readed from MWM we delete VertexArrayBuffer on BackendRenderer
    /// in this case m_VAO will be equal a 0
    /// also m_VAO == 0 will be on device that not support OES_vertex_array_object extension
    GLFunctions::glDeleteVertexArray(m_VAO);
  }
}

void VertexArrayBuffer::Render()
{
  if (!m_buffers.empty())
  {
    ASSERT(!m_program.IsNull(), ("Somebody not call Build. It's very bad. Very very bad"));
    /// if OES_vertex_array_object is supported than all bindings already saved in VAO
    /// and we need only bind VAO. In Bind method have ASSERT("bind already called")
    if (GLExtensionsList::Instance().IsSupported(GLExtensionsList::VertexArrayObject))
      Bind();
    else
      BindBuffers();

    GLFunctions::glDrawElements(m_indexBuffer->GetCurrentSize());
  }
}

void VertexArrayBuffer::Build(RefPointer<GpuProgram> program)
{
  ASSERT(m_VAO == 0 && m_program.IsNull(), ("No-no-no! You can't rebuild VertexArrayBuffer"));
  m_program = program;
  /// if OES_vertex_array_object not supported, than buffers will be bind on each Render call
  if (!GLExtensionsList::Instance().IsSupported(GLExtensionsList::VertexArrayObject))
    return;

  if (m_buffers.empty())
    return;

  m_VAO = GLFunctions::glGenVertexArray();
  Bind();
  BindBuffers();
}

RefPointer<GLBuffer> VertexArrayBuffer::GetBuffer(const BindingInfo & bindingInfo)
{
  buffers_map_t::iterator it = m_buffers.find(bindingInfo);
  if (it == m_buffers.end())
  {
    MasterPointer<DataBuffer> & buffer = m_buffers[bindingInfo];
    buffer.Reset(new DataBuffer(bindingInfo.GetElementSize(), m_dataBufferSize));
    return buffer.GetRefPointer();
  }

  return it->second.GetRefPointer();
}

uint16_t VertexArrayBuffer::GetAvailableIndexCount() const
{
  return m_indexBuffer->GetAvailableSize();
}

uint16_t VertexArrayBuffer::GetAvailableVertexCount() const
{
  if (m_buffers.empty())
    return m_dataBufferSize;

#ifdef DEBUG
  buffers_map_t::const_iterator it = m_buffers.begin();
  uint16_t prev = it->second->GetAvailableSize();
  for (; it != m_buffers.end(); ++it)
    ASSERT(prev == it->second->GetAvailableSize(), ());
#endif

  return m_buffers.begin()->second->GetAvailableSize();
}

uint16_t VertexArrayBuffer::GetStartIndexValue() const
{
  if (m_buffers.empty())
    return 0;

#ifdef DEBUG
  buffers_map_t::const_iterator it = m_buffers.begin();
  uint16_t prev = it->second->GetCurrentSize();
  for (; it != m_buffers.end(); ++it)
    ASSERT(prev == it->second->GetCurrentSize(), ());
#endif

  return m_buffers.begin()->second->GetCurrentSize();
}

void VertexArrayBuffer::UploadIndexes(uint16_t * data, uint16_t count)
{
  ASSERT(count <= m_indexBuffer->GetAvailableSize(), ());
  m_indexBuffer->UploadData(data, count);
}

void VertexArrayBuffer::Bind()
{
  ASSERT(m_VAO != 0, ("You need to call Build method before bind it and render"));
  GLFunctions::glBindVertexArray(m_VAO);
}

void VertexArrayBuffer::BindBuffers()
{
  buffers_map_t::iterator it = m_buffers.begin();
  for (; it != m_buffers.end(); ++it)
  {
    const BindingInfo & binding = it->first;
    RefPointer<DataBuffer> buffer = it->second.GetRefPointer();
    buffer->Bind();

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

  m_indexBuffer->Bind();
}
