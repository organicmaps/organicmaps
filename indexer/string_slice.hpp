#pragma once

#include "base/string_utils.hpp"

#include "std/cstdint.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace search
{
class StringSliceBase
{
public:
  using TString = strings::UniString;

  virtual ~StringSliceBase() = default;

  virtual TString const & Get(size_t i) const = 0;
  virtual size_t Size() const = 0;
};

class NoPrefixStringSlice : public StringSliceBase
{
public:
  NoPrefixStringSlice(vector<TString> const & strings) : m_strings(strings) {}

  virtual TString const & Get(size_t i) const override { return m_strings[i]; }
  virtual size_t Size() const override { return m_strings.size(); }

private:
  vector<TString> const & m_strings;
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

  inline bool operator==(JoinIterator const & rhs) const
  {
    return &m_slice == &rhs.m_slice && m_string == rhs.m_string && m_offset == rhs.m_offset;
  }

  inline bool operator!=(JoinIterator const & rhs) const { return !(*this == rhs); }

  inline strings::UniChar operator*() const { return GetChar(m_string, m_offset); }

  JoinIterator & operator++();

private:
  enum class Position
  {
    Begin,
    End
  };

  JoinIterator(StringSliceBase const & slice, Position position);

  void Normalize();

  size_t GetSize(size_t string) const;

  inline size_t GetMaxSize() const { return m_slice.Size() == 0 ? 0 : m_slice.Size() * 2 - 1; }

  strings::UniChar GetChar(size_t string, size_t offset) const;

  StringSliceBase const & m_slice;
  size_t m_string;
  size_t m_offset;
};
}  // namespace search
