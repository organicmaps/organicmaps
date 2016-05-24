#pragma once

#include "base/string_utils.hpp"

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
  NoPrefixStringSlice(vector<TString> const & strings)
      : m_strings(strings)
  {
  }

  virtual TString const & Get(size_t i) const override { return m_strings[i]; }
  virtual size_t Size() const override { return m_strings.size(); }

private:
  vector<TString> const & m_strings;
};
}  // namespace search
