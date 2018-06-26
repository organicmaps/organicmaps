#pragma once

#include "map/booking_filter.hpp"
#include "map/booking_filter_params.hpp"

#include "indexer/feature_decl.hpp"

#include "platform/safe_callback.hpp"

#include "base/macros.hpp"

#include <unordered_map>
#include <vector>

class DataSource;

namespace search
{
class Results;
}

namespace booking
{
class Api;

namespace filter
{

struct CachedResult
{
  CachedResult(Type type, std::vector<FeatureID> && featuresSorted)
    : m_type(type)
    , m_featuresSorted(featuresSorted)
  {
  }

  Type m_type;
  std::vector<FeatureID> m_featuresSorted;
};

using CachedResults = std::vector<CachedResult>;

using FillSearchMarksCallback =
    platform::SafeCallback<void(CachedResults results)>;

class FilterProcessor : public FilterBase::Delegate
{
public:
  FilterProcessor(DataSource const & dataSource, booking::Api const & api);

  void ApplyFilters(search::Results const & results, TasksInternal && tasks,
                    ApplicationMode const mode);

  void OnParamsUpdated(Type const type, std::shared_ptr<ParamsBase> const & params);

  void GetFeaturesFromCache(Types const & types, search::Results const & results,
                            FillSearchMarksCallback const & callback);

  // FilterInterface::Delegate overrides:
  DataSource const & GetDataSource() const override;
  Api const & GetApi() const override;

private:
  void ApplyConsecutively(search::Results const & results, TasksInternal & tasks);
  void ApplyIndependently(search::Results const & results, TasksInternal const & tasks);

  DataSource const & m_dataSource;
  Api const & m_api;

  std::unordered_map<Type, FilterPtr> m_filters;

  DISALLOW_COPY_AND_MOVE(FilterProcessor);
};
}  // namespace filter
}  // namespace booking
