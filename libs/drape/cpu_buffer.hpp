#pragma once

#include "drape/buffer_base.hpp"

#include "base/shared_buffer_manager.hpp"

namespace dp
{
class CPUBuffer : public BufferBase
{
  using TBase = BufferBase;

public:
  CPUBuffer(uint8_t elementSize, uint32_t capacity);
  ~CPUBuffer() override;
  CPUBuffer(CPUBuffer const &) = delete;
  CPUBuffer & operator=(CPUBuffer const &) = delete;
  CPUBuffer(CPUBuffer &&) noexcept = default;
  CPUBuffer & operator=(CPUBuffer &&) noexcept = default;

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

  SharedBufferManager::shared_buffer_ptr_t m_memory;
  unsigned char * m_memoryCursor;
};
}  // namespace dp
