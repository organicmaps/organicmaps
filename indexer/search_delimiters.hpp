#pragma once

#include "../base/string_utils.hpp"

namespace search
{
  class Delimiters
  {
  public:
    bool operator()(strings::UniChar c) const;
  };

  class CategoryDelimiters : public Delimiters
  {
  public:
    bool operator()(strings::UniChar c) const;
  };
}
