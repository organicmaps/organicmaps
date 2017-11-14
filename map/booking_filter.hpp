#pragma once

#include "map/booking_filter_availability_params.hpp"
#include "map/booking_filter_cache.hpp"

#include "base/worker_thread.hpp"

class Index;

namespace search
{
class Results;
}

namespace booking
{
class Api;

namespace filter
{
class Filter
{
public:
  Filter(Index const & index, booking::Api const & api);

  void Availability(search::Results const & results,
                    availability::internal::Params && params);

private:
  Index const & m_index;
  Api const & m_api;

  std::mutex m_mutex;
  availability::Cache m_availabilityCache;

  // |m_cacheDropCounter| and |m_currentParams| are used to identify request actuality.
  uint32_t m_cacheDropCounter = {};
  AvailabilityParams m_currentParams;
};
}  // namespace filter
}  // namespace booking
