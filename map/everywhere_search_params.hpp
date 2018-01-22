#pragma once

#include "map/booking_filter_availability_params.hpp"

#include "search/everywhere_search_callback.hpp"
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
  booking::filter::availability::Params m_bookingFilterParams;

  EverywhereSearchCallback::OnResults m_onResults;
};
}  // namespace search
