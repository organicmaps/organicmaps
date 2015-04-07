#include "base/SRC_FIRST.hpp"

#include "base/resource_pool.hpp"

BasePoolElemFactory::BasePoolElemFactory(char const * resName,
                                         size_t elemSize,
                                         size_t batchSize)
  : m_resName(resName),
    m_elemSize(elemSize),
    m_batchSize(batchSize)
{}

char const * BasePoolElemFactory::ResName() const
{
  return m_resName.c_str();
}

size_t BasePoolElemFactory::ElemSize() const
{
  return m_elemSize;
}

size_t BasePoolElemFactory::BatchSize() const
{
  return m_batchSize;
}

