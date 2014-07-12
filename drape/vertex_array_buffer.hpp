#pragma once

#include "index_buffer_mutator.hpp"
#include "attribute_buffer_mutator.hpp"
#include "pointers.hpp"
#include "index_buffer.hpp"
#include "data_buffer.hpp"
#include "binding_info.hpp"
#include "gpu_program.hpp"

#include "../std/map.hpp"

class VertexArrayBuffer
{
  typedef map<BindingInfo, MasterPointer<DataBuffer> > buffers_map_t;
public:
  VertexArrayBuffer(uint32_t indexBufferSize, uint32_t dataBufferSize);
  ~VertexArrayBuffer();

  ///{@
  /// On devices where implemented OES_vertex_array_object extensions we use it for build VertexArrayBuffer
  /// OES_vertex_array_object create OpenGL resource that belong only one GL context (which was created by)
  /// by this reason Build/Bind and Render must be called only on Frontendrendere thread
  void Render();
  void Build(RefPointer<GpuProgram> program);
  ///@}

  uint16_t GetAvailableVertexCount() const;
  uint16_t GetAvailableIndexCount() const;
  uint16_t GetStartIndexValue() const;
  uint16_t GetDynamicBufferOffset(BindingInfo const & bindingInfo);
  bool IsFilled() const;

  void UploadData(BindingInfo const & bindingInfo, void const * data, uint16_t count);
  void UploadIndexes(uint16_t const * data, uint16_t count);

  void ApplyMutation(RefPointer<IndexBufferMutator> indexMutator,
                     RefPointer<AttributeBufferMutator> attrMutator);

private:
  RefPointer<DataBuffer> GetOrCreateStaticBuffer(BindingInfo const & bindingInfo);
  RefPointer<DataBuffer> GetOrCreateDynamicBuffer(BindingInfo const & bindingInfo);
  RefPointer<DataBuffer> GetDynamicBuffer(BindingInfo const & bindingInfo) const;

  RefPointer<DataBuffer> GetOrCreateBuffer(BindingInfo const & bindingInfo, buffers_map_t & buffers);
  RefPointer<DataBuffer> GetBuffer(BindingInfo const & bindingInfo, buffers_map_t const & buffers) const;
  void Bind();
  void BindStaticBuffers();
  void BindDynamicBuffers();
  void BindBuffers(buffers_map_t const & buffers);

private:
  int m_VAO;
  buffers_map_t m_staticBuffers;
  buffers_map_t m_dynamicBuffers;

  MasterPointer<IndexBuffer> m_indexBuffer;
  uint32_t m_dataBufferSize;

  RefPointer<GpuProgram> m_program;
};
