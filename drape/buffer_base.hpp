#pragma once

#include "std/cstdint.hpp"

namespace dp
{

class BufferBase
{
public:
  BufferBase(uint8_t elementSize, uint16_t capacity);

  uint16_t GetCapacity() const;
  uint16_t GetCurrentSize() const;
  uint16_t GetAvailableSize() const;

  uint8_t GetElementSize() const;

  void Seek(uint16_t elementNumber);

protected:
  void Resize(uint16_t elementCount);
  void UploadData(uint16_t elementCount);
  void SetDataSize(uint16_t elementCount);

private:
  uint8_t m_elementSize;
  uint16_t m_capacity;
  uint16_t m_size;
};

} // namespace dp
