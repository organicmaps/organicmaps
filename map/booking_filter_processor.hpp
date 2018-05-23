#pragma once

#include "map/booking_filter.hpp"
#include "map/booking_filter_params.hpp"

#include "indexer/feature_decl.hpp"

#include "platform/safe_callback.hpp"

#include "base/macros.hpp"

#include <unordered_map>
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
enum class Type
{
  Deals,
  Availability
};

struct FilterTask
{
  FilterTask(Type const type, ParamsInternal && params)
    : m_type(type)
    , m_params(std::move(params))
  {
  }

  Type const m_type;
  ParamsInternal m_params;
};

using FillSearchMarksCallback = platform::SafeCallback<void(std::vector<FeatureID> availableHotelsSorted)>;

class FilterProcessor : public FilterBase::Delegate
{
public:
  FilterProcessor(Index const & index, booking::Api const & api);

  void ApplyFilters(search::Results const & results, std::vector<FilterTask> && tasks);

  void OnParamsUpdated(Type const type, std::shared_ptr<ParamsBase> const & params);

  void GetFeaturesFromCache(Type const type, search::Results const & results,
                            FillSearchMarksCallback const & callback);

  // FilterInterface::Delegate overrides:
  Index const & GetIndex() const override;
  Api const & GetApi() const override;

private:
  Index const & m_index;
  Api const & m_api;

  std::unordered_map<Type, FilterPtr> m_filters;

  DISALLOW_COPY_AND_MOVE(FilterProcessor);
};
}  // namespace filter
}  // namespace booking
