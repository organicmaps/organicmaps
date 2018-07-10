#pragma once

#include "map/booking_filter_params.hpp"
#include "map/search_product_info.hpp"

#include "search/hotels_filter.hpp"
#include "search/result.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace search
{
struct EverywhereSearchParams
{
  std::string m_query;
  std::string m_inputLocale;
  std::shared_ptr<hotels_filter::Rule> m_hotelsFilter;
  booking::filter::Tasks m_bookingFilterTasks;

  using OnResults =
      std::function<void(Results const & results, std::vector<ProductInfo> const & productInfo)>;

  OnResults m_onResults;
};
}  // namespace search
