#pragma once

#include "map/booking_filter_params.hpp"
#include "map/search_product_info.hpp"

#include "search/hotels_filter.hpp"
#include "search/result.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <boost/optional.hpp>

namespace search
{
struct EverywhereSearchParams
{
  using OnResults =
      std::function<void(Results const & results, std::vector<ProductInfo> const & productInfo)>;

  std::string m_query;
  std::string m_inputLocale;
  std::shared_ptr<hotels_filter::Rule> m_hotelsFilter;
  booking::filter::Tasks m_bookingFilterTasks;
  boost::optional<std::chrono::steady_clock::duration> m_timeout;

  OnResults m_onResults;
};
}  // namespace search
