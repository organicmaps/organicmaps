#pragma once

#include "map/booking_filter_availability_params.hpp"
#include "map/booking_filter_cache.hpp"

#include "base/macros.hpp"

#include <functional>
#include <memory>
#include <vector>

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

using FillSearchMarksCallback = platform::SafeCallback<void(std::vector<FeatureID> availableHotelsSorted)>;

class Filter
{
public:
  Filter(Index const & index, booking::Api const & api);

  void FilterAvailability(search::Results const & results,
                          availability::internal::Params const & params);

  void OnParamsUpdated(AvailabilityParams const & params);

  void GetAvailableFeaturesFromCache(search::Results const & results,
                                     FillSearchMarksCallback const & callback);

private:

  void UpdateAvailabilityParams(AvailabilityParams params);

  Index const & m_index;
  Api const & m_api;

  using CachePtr = std::shared_ptr<availability::Cache>;

  CachePtr m_availabilityCache = std::make_shared<availability::Cache>();

  AvailabilityParams m_currentParams;

  DISALLOW_COPY_AND_MOVE(Filter);
};
}  // namespace filter
}  // namespace booking
