#include "index_buffer.hpp"

IndexBuffer::IndexBuffer(uint16_t capacity)
  : GLBuffer(GLBuffer::IndexBuffer, sizeof(uint16_t), capacity)
{
}

void IndexBuffer::UploadData(uint16_t * data, uint16_t size)
{
  GLBuffer::UploadData((void *)data, size);
}
