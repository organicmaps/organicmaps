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

  void Bind();
  void Unbind();
  void Render();

  void BuildVertexArray(WeakPointer<GpuProgram> program);

  WeakPointer<GLBuffer> GetBuffer(const BindingInfo & bindingInfo);

  uint16_t GetAvailableVertexCount() const;
  uint16_t GetAvailableIndexCount() const;
  uint16_t GetStartIndexValue() const;
  void UploadIndexes(uint16_t * data, uint16_t count);

private:
  int m_VAO;
  typedef map<BindingInfo, StrongPointer<DataBuffer> > buffers_map_t;
  buffers_map_t m_buffers;

  StrongPointer<IndexBuffer> m_indexBuffer;
  uint32_t m_dataBufferSize;
};
