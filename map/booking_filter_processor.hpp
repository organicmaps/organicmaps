#pragma once

#include "map/booking_filter.hpp"
#include "map/booking_filter_params.hpp"

#include "indexer/feature_decl.hpp"

#include "platform/safe_callback.hpp"

#include "base/macros.hpp"

#include <unordered_map>
#include <utility>
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

struct CachedResult : ResultInternal<std::vector<FeatureID>>
{
  CachedResult(Type type, std::vector<FeatureID> && featuresSorted, std::vector<Extras> && extras,
               std::vector<FeatureID> && filteredOutSorted)
    : ResultInternal(std::move(featuresSorted), std::move(extras), std::move(filteredOutSorted))
    , m_type(type)
  {
  }

  Type m_type;
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

  void ApplyFilters(std::vector<FeatureID> && featureIds, TasksRawInternal && tasks,
                    ApplicationMode const mode);

  void OnParamsUpdated(Type const type, std::shared_ptr<ParamsBase> const & params);

  void GetFeaturesFromCache(Types const & types, search::Results const & results,
                            FillSearchMarksCallback const & callback);

  // FilterInterface::Delegate overrides:
  DataSource const & GetDataSource() const override;
  Api const & GetApi() const override;

private:
  template <typename Source, typename TaskInternalType>
  void ApplyFiltersInternal(Source && source, TaskInternalType && tasks,
                            ApplicationMode const mode);

  template <typename Source, typename TaskInternalType>
  void ApplyConsecutively(Source const & source, TaskInternalType & tasks);
  
  template <typename Source, typename TaskInternalType>
  void ApplyIndependently(Source const & source, TaskInternalType const & tasks);

  DataSource const & m_dataSource;
  Api const & m_api;

  std::unordered_map<Type, FilterPtr> m_filters;

  DISALLOW_COPY_AND_MOVE(FilterProcessor);
};
}  // namespace filter
}  // namespace booking
