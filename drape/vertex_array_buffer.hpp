#pragma once

#include "drape/index_buffer_mutator.hpp"
#include "drape/attribute_buffer_mutator.hpp"
#include "drape/pointers.hpp"
#include "drape/index_buffer.hpp"
#include "drape/data_buffer.hpp"
#include "drape/binding_info.hpp"
#include "drape/gpu_program.hpp"

#include "std/map.hpp"

namespace dp
{

struct IndicesRange
{
  uint16_t m_idxStart;
  uint16_t m_idxCount;
};

class VertexArrayBuffer
{
  typedef map<BindingInfo, MasterPointer<DataBuffer> > TBuffersMap;
public:
  VertexArrayBuffer(uint32_t indexBufferSize, uint32_t dataBufferSize);
  ~VertexArrayBuffer();

  /// This method must be call on reading thread, before VAO will be transfer on render thread
  void Preflush();

  ///{@
  /// On devices where implemented OES_vertex_array_object extensions we use it for build VertexArrayBuffer
  /// OES_vertex_array_object create OpenGL resource that belong only one GL context (which was created by)
  /// by this reason Build/Bind and Render must be called only on Frontendrendere thread
  void Render();
  void RenderRange(IndicesRange const & range);
  void Build(RefPointer<GpuProgram> program);
  ///@}

  uint16_t GetAvailableVertexCount() const;
  uint16_t GetAvailableIndexCount() const;
  uint16_t GetStartIndexValue() const;
  uint16_t GetDynamicBufferOffset(BindingInfo const & bindingInfo);
  uint16_t GetIndexCount() const;
  bool IsFilled() const;

  void UploadData(BindingInfo const & bindingInfo, void const * data, uint16_t count);
  void UploadIndexes(uint16_t const * data, uint16_t count);

  void ApplyMutation(RefPointer<IndexBufferMutator> indexMutator,
                     RefPointer<AttributeBufferMutator> attrMutator);

private:
  RefPointer<DataBuffer> GetOrCreateStaticBuffer(BindingInfo const & bindingInfo);
  RefPointer<DataBuffer> GetOrCreateDynamicBuffer(BindingInfo const & bindingInfo);
  RefPointer<DataBuffer> GetDynamicBuffer(BindingInfo const & bindingInfo) const;

  RefPointer<DataBuffer> GetOrCreateBuffer(BindingInfo const & bindingInfo, bool isDynamic);
  RefPointer<DataBuffer> GetBuffer(BindingInfo const & bindingInfo, bool isDynamic) const;
  void Bind() const;
  void BindStaticBuffers() const;
  void BindDynamicBuffers() const;
  void BindBuffers(TBuffersMap const & buffers) const;

private:
  int m_VAO;
  TBuffersMap m_staticBuffers;
  TBuffersMap m_dynamicBuffers;

  MasterPointer<IndexBuffer> m_indexBuffer;
  uint32_t m_dataBufferSize;

  RefPointer<GpuProgram> m_program;
};

} // namespace dp
