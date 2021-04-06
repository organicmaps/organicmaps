#pragma once

#include "search/result.hpp"

namespace search
{
struct ProductInfo
{
  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual ProductInfo GetProductInfo(Result const & result) const = 0;
  };
};
}
