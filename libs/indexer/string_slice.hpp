#pragma once

#include "base/string_utils.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace search
{
class StringSliceBase
{
public:
  using String = strings::UniString;

  virtual ~StringSliceBase() = default;

  virtual String const & Get(size_t i) const = 0;
  virtual size_t Size() const = 0;
};

class StringSlice : public StringSliceBase
{
public:
  StringSlice(std::vector<String> const & strings) : m_strings(strings) {}

  virtual String const & Get(size_t i) const override { return m_strings[i]; }
  virtual size_t Size() const override { return m_strings.size(); }

private:
  std::vector<String> const & m_strings;
};

// Allows to iterate over space-separated strings in StringSliceBase.
class JoinIterator
{
public:
  using difference_type = ptrdiff_t;
  using value_type = strings::UniChar;
  using pointer = strings::UniChar *;
  using reference = strings::UniChar &;
  using iterator_category = std::forward_iterator_tag;

  static JoinIterator Begin(StringSliceBase const & slice);
  static JoinIterator End(StringSliceBase const & slice);

  bool operator==(JoinIterator const & rhs) const
  {
    return &m_slice == &rhs.m_slice && m_string == rhs.m_string && m_offset == rhs.m_offset;
  }

  bool operator!=(JoinIterator const & rhs) const { return !(*this == rhs); }

  value_type operator*() const { return GetChar(m_string, m_offset); }

  JoinIterator & operator++();

private:
  enum class Position
  {
    Begin,
    End
  };

  JoinIterator(StringSliceBase const & slice, Position position);

  // Normalizes current position, i.e. moves to the next valid
  // character if current position is invalid, or to the end of the
  // whole sequence if there are no valid positions.
  void Normalize();

  size_t GetSize(size_t string) const;

  size_t GetMaxSize() const { return m_slice.Size() == 0 ? 0 : m_slice.Size() * 2 - 1; }

  value_type GetChar(size_t string, size_t offset) const;

  StringSliceBase const & m_slice;

  // Denotes the current string the iterator points to.  Even values
  // of |m_string| divided by two correspond to indices in
  // |m_slice|. Odd values correspond to intermediate space
  // characters.
  size_t m_string;

  // An index of the current character in the current string.
  size_t m_offset;
};
}  // namespace search
