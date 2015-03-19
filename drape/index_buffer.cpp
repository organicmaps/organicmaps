#include "drape/index_buffer.hpp"

namespace dp
{

IndexBuffer::IndexBuffer(uint16_t capacity)
  : DataBuffer(sizeof(uint16_t), capacity)
{
}

void IndexBuffer::UploadData(uint16_t const * data, uint16_t size)
{
  GetBuffer()->UploadData((void const *)data, size);
}

void IndexBuffer::UpdateData(uint16_t const * data, uint16_t size)
{
  ASSERT_LESS_OR_EQUAL(size, GetBuffer()->GetCapacity(), ());

  GetBuffer()->Seek(0);
  if (size > 0)
    UploadData(data, size);
}

} // namespace dp
