#pragma once

#include "map/discovery/discovery_search_params.hpp"
#include "map/search_product_info.hpp"

#include "search/result.hpp"

#include <functional>
#include <vector>

namespace search
{
class DiscoverySearchCallback
{
public:
  DiscoverySearchCallback(ProductInfo::Delegate & delegate,
                          DiscoverySearchParams::OnResults onResults);

  void operator()(Results const & results);

private:
  ProductInfo::Delegate & m_delegate;
  DiscoverySearchParams::OnResults m_onResults;
  std::vector<ProductInfo> m_productInfo;
};
}  // namespace search
