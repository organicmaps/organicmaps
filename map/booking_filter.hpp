#pragma once

#include "map/booking_filter_availability_params.hpp"
#include "map/booking_filter_cache.hpp"

#include <memory>

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
                          availability::internal::Params const & params);

private:
  Index const & m_index;
  Api const & m_api;

  using CachePtr = std::shared_ptr<availability::Cache>;

  CachePtr m_availabilityCache = std::make_shared<availability::Cache>();

  AvailabilityParams m_currentParams;
};
}  // namespace filter
}  // namespace booking
