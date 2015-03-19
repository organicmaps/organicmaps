#pragma once

#include "drape/buffer_base.hpp"

#include "std/vector.hpp"
#include "std/shared_ptr.hpp"

namespace dp
{

class CPUBuffer : public BufferBase
{
  typedef BufferBase TBase;
public:
  CPUBuffer(uint8_t elementSize, uint16_t capacity);
  ~CPUBuffer();

  void UploadData(void const * data, uint16_t elementCount, bool const advanceCursor);
  // Set memory cursor on element with number == "elementNumber"
  // Element numbers start from 0
  void Seek(uint16_t elementNumber);
  // Check function. In real world use must use it only in assert
  uint16_t GetCurrentElementNumber() const;
  unsigned char const * Data() const;

private:
  unsigned char * NonConstData();
  unsigned char * GetCursor() const;

private:
  unsigned char * m_memoryCursor;
  shared_ptr<vector<unsigned char> > m_memory;
};

} //namespace dp
