#include "drape/index_buffer.hpp"
#include "drape/index_storage.hpp"

#include "base/assert.hpp"

namespace dp
{
IndexBuffer::IndexBuffer(uint32_t capacity) : DataBuffer((uint8_t)IndexStorage::SizeOfIndex(), capacity) {}

void IndexBuffer::UploadData(ref_ptr<GraphicsContext> context, void const * data, uint32_t size)
{
  GetBuffer()->UploadData(context, data, size);
}

void IndexBuffer::UpdateData(ref_ptr<GraphicsContext> context, void const * data, uint32_t size)
{
  ASSERT_LESS_OR_EQUAL(size, GetBuffer()->GetCapacity(), ());

  GetBuffer()->Seek(0);
  if (size > 0)
    UploadData(context, data, size);
}
}  // namespace dp
