#pragma once

#include "base/string_utils.hpp"

namespace search
{
  class Delimiters
  {
  public:
    bool operator()(strings::UniChar c) const;
  };
}
