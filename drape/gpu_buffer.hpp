#pragma once

#include "drape/pointers.hpp"
#include "drape/buffer_base.hpp"

namespace dp
{

class GPUBuffer : public BufferBase
{
  typedef BufferBase TBase;
public:
  enum Target
  {
    ElementBuffer,
    IndexBuffer
  };

public:
  GPUBuffer(Target t, void const * data, uint8_t elementSize, uint32_t capacity);
  ~GPUBuffer();

  void UploadData(void const * data, uint32_t elementCount);
  void Bind();

  void * Map();
  void UpdateData(void * gpuPtr, void const * data, uint32_t elementOffset, uint32_t elementCount);
  void Unmap();

protected:
  /// discard old data
  void Resize(void const * data, uint32_t elementCount);

private:
  friend class GPUBufferMapper;
  Target m_t;
  uint32_t m_bufferID;

#ifdef DEBUG
  bool m_isMapped;
#endif
};

} // namespace dp
