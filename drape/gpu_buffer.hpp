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

  void UploadData(const void * data, uint16_t elementCount);
  void Bind();

private:
  Target m_t;
  uint32_t m_bufferID;
};
