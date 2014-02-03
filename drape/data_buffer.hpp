#pragma once

#include "gpu_buffer.hpp"

class DataBuffer : public GPUBuffer
{
public:
  DataBuffer(uint8_t elementSize, uint16_t capacity);
};

