#pragma once

#include "search/result.hpp"

#include <functional>
#include <vector>

namespace search
{
struct ProductInfo
{
  static auto constexpr kInvalidRating = kInvalidRatingValue;

  bool m_isLocalAdsCustomer = false;
  float m_ugcRating = kInvalidRating;
};
// An on-results-callback that should be used for search over all
// maps.
//
// *NOTE* the class is NOT thread safe.
class EverywhereSearchCallback
{
public:
  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual ProductInfo GetProductInfo(Result const & result) const = 0;
  };

  // The signature of the callback should be the same as EverywhereSaerchParams::OnResults, but
  // EverywhereSaerchParams is located in map project and we do not need dependency.
  using OnResults =
      std::function<void(Results const & results, std::vector<ProductInfo> const & productInfo)>;

  EverywhereSearchCallback(Delegate & delegate, OnResults onResults);

  void operator()(Results const & results);

private:
  Delegate & m_delegate;
  OnResults m_onResults;
  std::vector<ProductInfo> m_productInfo;
};
}  // namespace search
