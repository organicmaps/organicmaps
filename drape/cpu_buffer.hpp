#pragma once

#include "buffer_base.hpp"

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

class CPUBuffer : public BufferBase
{
  typedef BufferBase base_t;
public:
  CPUBuffer(uint8_t elementSize, uint16_t capacity);
  ~CPUBuffer();

  void UploadData(const void * data, uint16_t elementCount);
  // Set memory cursor on element with number == "elementNumber"
  // Element numbers start from 0
  void Seek(uint16_t elementNumber);
  // Check function. In real world use must use it only in assert
  uint16_t GetCurrentElementNumber() const;
  const unsigned char * Data() const;

private:
  unsigned char * GetCursor() const;

private:
  unsigned char * m_memoryCursor;
  shared_ptr<vector<unsigned char> > m_memory;
};
