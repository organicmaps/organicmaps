#pragma once

#include "gpu_buffer.hpp"

class IndexBuffer : public GPUBuffer
{
public:
  IndexBuffer(uint16_t capacity);

  void UploadData(uint16_t * data, uint16_t size);
};
