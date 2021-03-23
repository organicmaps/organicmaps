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

  static auto constexpr kInvalidRating = kInvalidRatingValue;

  float m_ugcRating = kInvalidRating;
};
}
