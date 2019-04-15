#pragma once

#include "map/booking_filter_params.hpp"

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
  using OnResults = std::function<void(Results const & results)>;

  std::string m_query;
  std::string m_inputLocale;
  std::shared_ptr<hotels_filter::Rule> m_hotelsFilter;
  booking::filter::Tasks m_bookingFilterTasks;

  OnStarted m_onStarted;
  OnResults m_onCompleted;
};
}  // namespace search
