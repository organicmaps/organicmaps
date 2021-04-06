#pragma once

#include "map/everywhere_search_params.hpp"
#include "map/search_product_info.hpp"

#include "search/result.hpp"

#include <functional>
#include <vector>

namespace search
{
// An on-results-callback that should be used for search over all
// maps.
//
// *NOTE* the class is NOT thread safe.
class EverywhereSearchCallback
{
public:
  EverywhereSearchCallback(ProductInfo::Delegate & productInfoDelegate,
                           EverywhereSearchParams::OnResults onResults);

  void operator()(Results const & results);

private:
  ProductInfo::Delegate & m_productInfoDelegate;
  EverywhereSearchParams::OnResults m_onResults;
  std::vector<ProductInfo> m_productInfo;
};
}  // namespace search
