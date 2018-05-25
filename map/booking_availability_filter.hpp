#pragma once

#include "partners_api/booking_availability_params.hpp"

#include "map/booking_filter.hpp"
#include "map/booking_filter_cache.hpp"
#include "map/booking_filter_params.hpp"

#include <memory>

namespace search
{
class Results;
}

namespace booking
{
namespace filter
{
class AvailabilityFilter : public FilterBase
{
public:
  explicit AvailabilityFilter(Delegate const & d);
  void ApplyFilter(search::Results const & results, ParamsInternal const & filterParams) override;

  void GetFeaturesFromCache(search::Results const & results,
                            std::vector<FeatureID> & sortedResults) override;
  void UpdateParams(ParamsBase const & apiParams) override;

private:
  using CachePtr = std::shared_ptr<availability::Cache>;
  CachePtr m_cache = std::make_shared<availability::Cache>();

  AvailabilityParams m_apiParams;
};
}  // namespace filter
}  // namespace booking
