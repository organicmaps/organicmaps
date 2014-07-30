#include "index_buffer.hpp"

namespace dp
{

IndexBuffer::IndexBuffer(uint16_t capacity)
  : GPUBuffer(GPUBuffer::IndexBuffer, sizeof(uint16_t), capacity)
{
}

void IndexBuffer::UploadData(uint16_t const * data, uint16_t size)
{
  GPUBuffer::UploadData((void *)data, size);
}

void IndexBuffer::UpdateData(uint16_t const * data, uint16_t size)
{
  GPUBuffer::Resize(size);
  UploadData(data, size);
}

} // namespace dp
