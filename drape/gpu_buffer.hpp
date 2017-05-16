#pragma once

#include "drape/buffer_base.hpp"
#include "drape/pointers.hpp"

namespace dp
{
class GPUBuffer : public BufferBase
{
  using TBase = BufferBase;

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

  void * Map(uint32_t elementOffset, uint32_t elementCount);
  void UpdateData(void * gpuPtr, void const * data, uint32_t elementOffset, uint32_t elementCount);
  void Unmap();

protected:
  // Discard old data.
  void Resize(void const * data, uint32_t elementCount);

private:
  friend class GPUBufferMapper;
  Target m_t;
  uint32_t m_bufferID;
  uint32_t m_mappingOffset;

#ifdef DEBUG
  bool m_isMapped;
#endif
};
}  // namespace dp
