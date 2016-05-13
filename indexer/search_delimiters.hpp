#pragma once

#include "base/string_utils.hpp"

namespace search
{
class Delimiters
{
public:
  bool operator()(strings::UniChar c) const;
};

class DelimitersWithExceptions
{
public:
  DelimitersWithExceptions(vector<strings::UniChar> const & exceptions);

  bool operator()(strings::UniChar c) const;

private:
  vector<strings::UniChar> m_exceptions;
  Delimiters m_delimiters;
};
}  // namespace search
