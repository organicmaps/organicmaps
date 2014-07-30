#pragma once

#include "gpu_buffer.hpp"

namespace dp
{

class DataBuffer : public GPUBuffer
{
public:
  DataBuffer(uint8_t elementSize, uint16_t capacity);
};

} // namespace dp

