#pragma once

#include "map/booking_filter_params.hpp"

#include "search/hotels_filter.hpp"

#include <functional>
#include <memory>
#include <optional>
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
  booking::filter::Tasks m_bookingFilterTasks;
  std::optional<std::chrono::steady_clock::duration> m_timeout;

  OnStarted m_onStarted;
  OnCompleted m_onCompleted;
};
}  // namespace search
