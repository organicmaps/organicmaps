#pragma once

#include "map/discovery/discovery_client_params.hpp"
#include "map/search_product_info.hpp"

#include "search/result.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace discovery
{
struct DiscoverySearchParams
{
  using OnResults = std::function<void(search::Results const & results,
                                       std::vector<search::ProductInfo> const & productInfo)>;

  std::string m_query;
  size_t m_itemsCount = 0;
  m2::RectD m_viewport;
  OnResults m_onResults = nullptr;
};
}  // namespace discovery
