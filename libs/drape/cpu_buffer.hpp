#pragma once

#include "drape/buffer_base.hpp"

#include <memory>
#include <vector>

namespace dp
{
class CPUBuffer : public BufferBase
{
  using TBase = BufferBase;

public:
  CPUBuffer(uint8_t elementSize, uint32_t capacity);
  ~CPUBuffer() override;

  void UploadData(void const * data, uint32_t elementCount);
  // Set memory cursor on element with number == "elementNumber"
  // Element numbers start from 0
  void Seek(uint32_t elementNumber) override;
  // Check function. In real world use must use it only in assert
  uint32_t GetCurrentElementNumber() const;
  unsigned char const * Data() const;

private:
  unsigned char * NonConstData();
  unsigned char * GetCursor() const;

  unsigned char * m_memoryCursor;
  std::shared_ptr<std::vector<unsigned char>> m_memory;
};
}  // namespace dp
