#include "data_buffer.hpp"

namespace dp
{

DataBuffer::DataBuffer(uint8_t elementSize, uint16_t capacity)
  : GPUBuffer(GPUBuffer::ElementBuffer, elementSize, capacity)
{
}

}
