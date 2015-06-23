#include "drape/index_storage.hpp"
#include "drape/glextensions_list.hpp"

#include "std/utility.hpp"

namespace dp
{

IndexStorage::IndexStorage()
  : m_size(0)
{}

IndexStorage::IndexStorage(vector<uint32_t> && initial)
{
  m_size = (uint32_t)initial.size();
  if (IsSupported32bit())
  {
    m_storage = move(initial);
  }
  else
  {
    /// we pack 2 uint16_t indices into single m_storage element
    /// every element of "initial" vector is single index
    m_storage.resize(GetStorageSize(m_size));
    for (size_t i = 0; i < initial.size(); i++)
    {
      uint16_t * ptr = reinterpret_cast<uint16_t *>(m_storage.data()) + i;
      *ptr = (uint16_t)initial[i];
    }
  }
}

void IndexStorage::Resize(uint32_t size)
{
  m_size = size;
  m_storage.resize(GetStorageSize(m_size));
}

uint32_t IndexStorage::Size() const
{
  return m_size;
}

void * IndexStorage::GetRaw(uint32_t offsetInElements)
{
  if (IsSupported32bit())
    return &m_storage[offsetInElements];

  return reinterpret_cast<uint16_t *>(m_storage.data()) + offsetInElements;
}

void const * IndexStorage::GetRawConst() const
{
  return static_cast<void const *>(m_storage.data());
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

uint32_t IndexStorage::GetStorageSize(uint32_t elementsCount) const
{
  return IsSupported32bit() ? elementsCount : (elementsCount / 2 + 1);
}

}
