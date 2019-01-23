#include "drape/index_buffer.hpp"
#include "drape/index_storage.hpp"

#include "base/assert.hpp"

namespace dp
{
IndexBuffer::IndexBuffer(uint32_t capacity)
  : DataBuffer((uint8_t)IndexStorage::SizeOfIndex(), capacity)
{}

void IndexBuffer::UploadData(ref_ptr<GraphicsContext> context, void const * data, uint32_t size)
{
  GetBuffer()->UploadData(context, data, size);
}

void IndexBuffer::UpdateData(ref_ptr<GraphicsContext> context, void const * data, uint32_t size)
{
  ASSERT_LESS_OR_EQUAL(size, GetBuffer()->GetCapacity(), ());
  if (size == 0)
    return;

  GetBuffer()->Seek(0);
  if (context->GetApiVersion() == dp::ApiVersion::Vulkan)
  {
    // For Vulkan we can't update by means of UploadData, because UploadData can be called from
    // BR, where command buffers are not available.
    auto ptr = GetBuffer()->Map(context, 0, size);
    CHECK(ptr != nullptr, ());
    GetBuffer()->UpdateData(ptr, data, 0, size);
    GetBuffer()->Unmap(context);
  }
  else
  {
    UploadData(context, data, size);
  }
}
}  // namespace dp
