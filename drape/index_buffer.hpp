#pragma once

#include "drape/data_buffer.hpp"

namespace dp
{

class IndexBuffer : public DataBuffer
{
public:
  IndexBuffer(uint32_t capacity);

  /// check size of buffer and size of uploaded data
  void UploadData(void const * data, uint32_t size);
  /// resize buffer to new size, and discard old data
  void UpdateData(void const * data, uint32_t size);
};

} // namespace dp
