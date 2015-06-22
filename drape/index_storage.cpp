#include "drape/index_storage.hpp"
#include "drape/glextensions_list.hpp"

#include "std/utility.hpp"

namespace dp
{

IndexStorage::IndexStorage(vector<uint32_t> && initial)
{
  if (IsSupported32bit())
  {
    m_storage32bit = move(initial);
  }
  else
  {
    m_storage16bit.reserve(initial.size());
    for (size_t i = 0; i < initial.size(); i++)
      m_storage16bit.push_back((uint16_t)initial[i]);
  }
}

void IndexStorage::Resize(uint32_t size)
{
  if (IsSupported32bit())
    m_storage32bit.resize(size);
  else
    m_storage16bit.resize(size);
}

uint32_t IndexStorage::Size() const
{
  return IsSupported32bit() ? (uint32_t)m_storage32bit.size() : (uint32_t)m_storage16bit.size();
}

void * IndexStorage::GetRaw(uint32_t offsetInElements)
{
  return IsSupported32bit() ? static_cast<void *>(&m_storage32bit[offsetInElements]) :
                              static_cast<void *>(&m_storage16bit[offsetInElements]);
}

void const * IndexStorage::GetRawConst() const
{
  return IsSupported32bit() ? static_cast<void const *>(m_storage32bit.data()) :
                              static_cast<void const *>(m_storage16bit.data());
}

bool IndexStorage::IsSupported32bit()
{
  static bool const supports32bit = GLExtensionsList::Instance().IsSupported(GLExtensionsList::UintIndices);
  return supports32bit;
}

uint32_t IndexStorage::SizeOfIndex()
{
  return IsSupported32bit() ? sizeof(uint32_t) : sizeof(uint16_t);
}

}
