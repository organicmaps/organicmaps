#include "../base/SRC_FIRST.hpp"

#include "resource_pool.hpp"

BasePoolElemFactory::BasePoolElemFactory(char const * resName, size_t elemSize)
  : m_resName(resName), m_elemSize(elemSize)
{}

char const * BasePoolElemFactory::ResName() const
{
  return m_resName.c_str();
}

size_t BasePoolElemFactory::ElemSize() const
{
  return m_elemSize;
}
