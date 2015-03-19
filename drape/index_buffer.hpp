#pragma once

#include "drape/data_buffer.hpp"

namespace dp
{

class IndexBuffer : public DataBuffer
{
public:
  IndexBuffer(uint16_t capacity);

  /// check size of buffer and size of uploaded data
  void UploadData(uint16_t const * data, uint16_t size);
  /// resize buffer to new size, and discard old data
  void UpdateData(uint16_t const * data, uint16_t size);
};

} // namespace dp
