#include "data_buffer.hpp"

DataBuffer::DataBuffer(uint8_t elementSize, uint16_t capacity)
  : GPUBuffer(GPUBuffer::ElementBuffer, elementSize, capacity)
{
}
