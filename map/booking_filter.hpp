#pragma once

#include "map/booking_filter_availability_params.hpp"
#include "map/booking_filter_cache.hpp"

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

  void FilterAvailability(search::Results const & results,
                          availability::internal::Params && params);

private:
  Index const & m_index;
  Api const & m_api;

  availability::Cache m_availabilityCache;

  // |m_cacheDropCounter| is used to identify request freshness.
  AvailabilityParams m_currentParams;
};
}  // namespace filter
}  // namespace booking
