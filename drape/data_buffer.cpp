#include "data_buffer.hpp"

DataBuffer::DataBuffer(uint16_t elementSize, uint16_t capacity)
  : GLBuffer(GLBuffer::ElementBuffer, elementSize, capacity)
{
}
