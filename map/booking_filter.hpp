#pragma once

#include "map/booking_filter_availability_params.hpp"
#include "map/booking_filter_cache.hpp"

#include "base/macros.hpp"

#include <memory>
#include <mutex>

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
// NOTE: this class IS thread-safe.
class Filter
{
public:
  Filter(Index const & index, booking::Api const & api);

  void FilterAvailability(search::Results const & results,
                          availability::internal::Params const & params);

  void OnParamsUpdated(AvailabilityParams const & params);

  availability::Cache::HotelStatus GetHotelAvailabilityStatus(std::string const & hotelId);

private:
  Index const & m_index;
  Api const & m_api;

  using CachePtr = std::shared_ptr<availability::Cache>;

  CachePtr m_availabilityCache = std::make_shared<availability::Cache>();

  AvailabilityParams m_currentParams;
  std::mutex m_mutex;

  DISALLOW_COPY_AND_MOVE(Filter);
};
}  // namespace filter
}  // namespace booking
