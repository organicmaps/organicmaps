#pragma once

#include "map/booking_filter_availability_params.hpp"

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
  using OnResults =
      std::function<void(Results const & results, std::vector<bool> const & isLocalAdsCustomer)>;

  std::string m_query;
  std::string m_inputLocale;
  std::shared_ptr<hotels_filter::Rule> m_hotelsFilter;
  booking::filter::availability::Params m_bookingFilterParams;

  OnResults m_onResults;
};
}  // namespace search
