#pragma once

#include "map/booking_filter_availability_params.hpp"
#include "map/booking_filter_cache.hpp"

namespace search
{
class Results;
}

class Index;

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
                    availability::internal::Params const & params) const;

private:
  Index const & m_index;
  Api const & m_api;
  mutable availability::Cache m_availabilityCache;
};
}  // namespace filter
}  // namespace booking
