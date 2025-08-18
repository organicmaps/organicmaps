#include "indexer/string_slice.hpp"

namespace search
{
JoinIterator::JoinIterator(StringSliceBase const & slice, Position position) : m_slice(slice)
{
  if (position == Position::Begin)
  {
    m_string = 0;
    m_offset = 0;
    Normalize();
  }
  else
  {
    m_string = GetMaxSize();
    m_offset = 0;
  }
}

// static
JoinIterator JoinIterator::Begin(StringSliceBase const & slice)
{
  return JoinIterator(slice, Position::Begin);
}

// static
JoinIterator JoinIterator::End(StringSliceBase const & slice)
{
  return JoinIterator(slice, Position::End);
}

JoinIterator & JoinIterator::operator++()
{
  ++m_offset;
  Normalize();
  return *this;
}

void JoinIterator::Normalize()
{
  while (m_string != GetMaxSize() && m_offset >= GetSize(m_string))
  {
    ++m_string;
    m_offset = 0;
  }
}

size_t JoinIterator::GetSize(size_t string) const
{
  if (string >= GetMaxSize())
    return 0;

  if (string & 1)
    return 1;

  return m_slice.Get(string >> 1).size();
}

JoinIterator::value_type JoinIterator::GetChar(size_t string, size_t offset) const
{
  if (string >= GetMaxSize())
    return 0;

  if (string & 1)
  {
    ASSERT_EQUAL(offset, 0, ());
    return ' ';
  }

  auto const & s = m_slice.Get(string >> 1);
  ASSERT_LESS(offset, s.size(), ());
  return s[offset];
}
}  // namespace search
