#pragma once

#include "pointers.hpp"
#include "index_buffer.hpp"
#include "data_buffer.hpp"
#include "binding_info.hpp"
#include "gpu_program.hpp"

#include "../std/map.hpp"

class VertexArrayBuffer
{
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
  bool IsFilled() const;

  void UploadData(BindingInfo const & bindingInfo, void const * data, uint16_t count);
  void UploadIndexes(uint16_t const * data, uint16_t count);

private:
  friend class IndexBufferMutator;
  void UpdateIndexBuffer(uint16_t const * data, uint16_t count);

private:
  RefPointer<DataBuffer> GetBuffer(const BindingInfo & bindingInfo);
  void Bind();
  void BindBuffers();

private:
  int m_VAO;
  typedef map<BindingInfo, MasterPointer<DataBuffer> > buffers_map_t;
  buffers_map_t m_buffers;

  MasterPointer<IndexBuffer> m_indexBuffer;
  uint32_t m_dataBufferSize;

  RefPointer<GpuProgram> m_program;
};
