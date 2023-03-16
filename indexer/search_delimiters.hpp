#pragma once

#include "base/string_utils.hpp"

#include <vector>

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
  explicit DelimitersWithExceptions(std::vector<strings::UniChar> const & exceptions);

  bool operator()(strings::UniChar c) const;

private:
  std::vector<strings::UniChar> m_exceptions;
  Delimiters m_delimiters;
};
}  // namespace search
