#include "drape/index_buffer.hpp"

namespace dp
{

IndexBuffer::IndexBuffer(uint16_t capacity)
  : DataBuffer(GPUBuffer::IndexBuffer, sizeof(uint16_t), capacity)
{
}

void IndexBuffer::UploadData(uint16_t const * data, uint16_t size)
{
  DataBuffer::UploadData((void *)data, size);
}

void IndexBuffer::UpdateData(uint16_t const * data, uint16_t size)
{
  ASSERT_LESS_OR_EQUAL(size, GetCapacity(), ());

  Seek(0);
  if (size > 0)
    UploadData(data, size);
}

} // namespace dp
