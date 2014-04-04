#pragma once

#include "buffer_base.hpp"

class GPUBuffer : public BufferBase
{
  typedef BufferBase base_t;
public:
  enum Target
  {
    ElementBuffer,
    IndexBuffer
  };

public:
  GPUBuffer(Target t, uint8_t elementSize, uint16_t capacity);
  ~GPUBuffer();

  void UploadData(void const * data, uint16_t elementCount);
  void Bind();

protected:
  /// discard old data
  void Resize(uint16_t elementCount);

private:
  Target m_t;
  uint32_t m_bufferID;
};
