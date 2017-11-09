#pragma once

#include "map/booking_filter_availability_params.hpp"

#include "search/hotels_filter.hpp"

#include <functional>
#include <memory>
#include <string>

namespace search
{
class Results;

struct ViewportSearchParams
{
  using OnStarted = std::function<void()>;
  using OnCompleted = std::function<void(Results const & results)>;

  std::string m_query;
  std::string m_inputLocale;
  std::shared_ptr<hotels_filter::Rule> m_hotelsFilter;
  booking::filter::availability::Params m_bookingFilterParams;

  OnStarted m_onStarted;
  OnCompleted m_onCompleted;
};
}  // namespace search
