#pragma once

#include "coding/reader.hpp"

/// Reader wrapper to avoid penalty on copy and polymorphic SubReader creation.
template <class ReaderT>
class SubReaderWrapper
{
  ReaderT * m_p;
  uint64_t m_pos;
  uint64_t m_size;

protected:
  SubReaderWrapper(ReaderT * p, uint64_t pos, uint64_t size) : m_p(p), m_pos(pos), m_size(size)
  {
    ASSERT_LESS_OR_EQUAL(pos + size, m_p->Size(), (pos, size));
  }

public:
  explicit SubReaderWrapper(ReaderT * p) : m_p(p), m_pos(0), m_size(p->Size()) {}

  uint64_t Size() const { return m_size; }

  void Read(uint64_t pos, void * p, size_t size) const
  {
    ASSERT_LESS_OR_EQUAL(pos + size, m_size, (pos, size));
    m_p->Read(pos + m_pos, p, size);
  }

  SubReaderWrapper SubReader(uint64_t pos, uint64_t size) const { return SubReaderWrapper(m_p, pos + m_pos, size); }
};

/// Non template reader source for regular functions with incapsulated implementation.
class ReaderSrc : public ReaderSource<SubReaderWrapper<Reader>>
{
  typedef SubReaderWrapper<Reader> ReaderT;
  typedef ReaderSource<ReaderT> BaseT;

public:
  explicit ReaderSrc(Reader & reader) : BaseT(ReaderT(&reader)) {}
  explicit ReaderSrc(Reader * reader) : BaseT(ReaderT(reader)) {}
};
