#pragma once

#include "std/cstdint.hpp"

namespace dp
{

class BufferBase
{
public:
  BufferBase(uint8_t elementSize, uint32_t capacity);

  uint32_t GetCapacity() const;
  uint32_t GetCurrentSize() const;
  uint32_t GetAvailableSize() const;

  uint8_t GetElementSize() const;

  void Seek(uint32_t elementNumber);

protected:
  void Resize(uint32_t elementCount);
  void UploadData(uint32_t elementCount);
  void SetDataSize(uint32_t elementCount);

private:
  uint8_t m_elementSize;
  uint32_t m_capacity;
  uint32_t m_size;
};

} // namespace dp
