#include "index_buffer.hpp"

IndexBuffer::IndexBuffer(uint16_t capacity)
  : GLBuffer(GLBuffer::IndexBuffer, sizeof(uint16_t), capacity)
{
}
