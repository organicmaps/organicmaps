#pragma once
#include "coding/reader.hpp"
#include "std/unique_ptr.hpp"

class PolymorphReader
{
public:
  // Takes ownership of pReader
  explicit PolymorphReader(Reader const * pReader = 0) : m_pReader(pReader)
  {
  }

  PolymorphReader(PolymorphReader const & reader)
    : m_pReader(reader.m_pReader->CreateSubReader(0, reader.m_pReader->Size()))
  {
  }

  PolymorphReader & operator = (PolymorphReader const & reader)
  {
    PolymorphReader(reader).m_pReader.swap(m_pReader);
    return *this;
  }

  inline uint64_t Size() const
  {
    return m_pReader->Size();
  }

  inline void Read(uint64_t pos, void * p, size_t size) const
  {
    m_pReader->Read(pos, p, size);
  }

  inline PolymorphReader SubReader(uint64_t pos, uint64_t size) const
  {
    return PolymorphReader(m_pReader->CreateSubReader(pos, size));
  }

private:
  unique_ptr<Reader const> m_pReader;
};
