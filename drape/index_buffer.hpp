#pragma once

#include "drape/data_buffer.hpp"

namespace dp
{
class IndexBuffer : public DataBuffer
{
public:
  explicit IndexBuffer(uint32_t capacity);

  // Check size of buffer and size of uploaded data.
  void UploadData(void const * data, uint32_t size);
  // Resize buffer to new size, and discard old data.
  void UpdateData(void const * data, uint32_t size);
};
}  // namespace dp
