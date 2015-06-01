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

  IndicesRange()
    : m_idxStart(0), m_idxCount(0)
  {}

  IndicesRange(uint16_t idxStart, uint16_t idxCount)
    : m_idxStart(idxStart), m_idxCount(idxCount)
  {}

  bool IsValid() const { return m_idxCount != 0; }
};

class VertexArrayBuffer
{
  typedef map<BindingInfo, drape_ptr<DataBuffer> > TBuffersMap;
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
  void Build(ref_ptr<GpuProgram> program);
  ///@}

  uint16_t GetAvailableVertexCount() const;
  uint16_t GetAvailableIndexCount() const;
  uint16_t GetStartIndexValue() const;
  uint16_t GetDynamicBufferOffset(BindingInfo const & bindingInfo);
  uint16_t GetIndexCount() const;

  void UploadData(BindingInfo const & bindingInfo, void const * data, uint16_t count);
  void UploadIndexes(uint16_t const * data, uint16_t count);

  void ApplyMutation(ref_ptr<IndexBufferMutator> indexMutator,
                     ref_ptr<AttributeBufferMutator> attrMutator);

private:
  ref_ptr<DataBuffer> GetOrCreateStaticBuffer(BindingInfo const & bindingInfo);
  ref_ptr<DataBuffer> GetOrCreateDynamicBuffer(BindingInfo const & bindingInfo);
  ref_ptr<DataBuffer> GetDynamicBuffer(BindingInfo const & bindingInfo) const;

  ref_ptr<DataBuffer> GetOrCreateBuffer(BindingInfo const & bindingInfo, bool isDynamic);
  ref_ptr<DataBuffer> GetBuffer(BindingInfo const & bindingInfo, bool isDynamic) const;
  void Bind() const;
  void BindStaticBuffers() const;
  void BindDynamicBuffers() const;
  void BindBuffers(TBuffersMap const & buffers) const;

  ref_ptr<DataBufferBase> GetIndexBuffer() const;

private:
  /// m_VAO - VertexArrayObject name/identificator
  int m_VAO;
  TBuffersMap m_staticBuffers;
  TBuffersMap m_dynamicBuffers;

  drape_ptr<IndexBuffer> m_indexBuffer;
  uint32_t m_dataBufferSize;

  ref_ptr<GpuProgram> m_program;
};

} // namespace dp
