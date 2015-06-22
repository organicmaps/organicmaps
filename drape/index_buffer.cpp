#include "drape/index_buffer.hpp"
#include "drape/index_storage.hpp"

namespace dp
{

IndexBuffer::IndexBuffer(uint32_t capacity)
  : DataBuffer((uint8_t)IndexStorage::SizeOfIndex(), capacity)
{
}

void IndexBuffer::UploadData(void const * data, uint32_t size)
{
  GetBuffer()->UploadData((void const *)data, size);
}

void IndexBuffer::UpdateData(void const * data, uint32_t size)
{
  ASSERT_LESS_OR_EQUAL(size, GetBuffer()->GetCapacity(), ());

  GetBuffer()->Seek(0);
  if (size > 0)
    UploadData(data, size);
}

} // namespace dp
