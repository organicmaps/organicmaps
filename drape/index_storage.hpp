#pragma once

#include "std/vector.hpp"

namespace dp
{

class IndexStorage
{
public:
  IndexStorage();
  IndexStorage(vector<uint32_t> && initial);

  uint32_t Size() const;
  void Resize(uint32_t size);

  void * GetRaw(uint32_t offsetInElements = 0);
  void const * GetRawConst() const;

  static bool IsSupported32bit();
  static uint32_t SizeOfIndex();

private:
  vector<uint32_t> m_storage;
  uint32_t m_size;

  uint32_t GetStorageSize(uint32_t elementsCount) const;
};


} // namespace dp
