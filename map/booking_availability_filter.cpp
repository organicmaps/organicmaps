#include "map/booking_availability_filter.hpp"

#include "search/result.hpp"

#include "partners_api/booking_api.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature_decl.hpp"

#include "platform/platform.hpp"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

using namespace booking::filter;

namespace
{
struct HotelToResult
{
  explicit HotelToResult(search::Result const & result) : m_result(result) {}

  search::Result m_result;
  std::string m_hotelId;
  availability::Cache::HotelStatus m_cacheStatus = availability::Cache::HotelStatus::Absent;
};

using HotelToResults = std::vector<HotelToResult>;

void UpdateCache(HotelToResults const & hotelToResults, std::vector<std::string> const & hotelIds,
                 availability::Cache & cache)
{
  using availability::Cache;

  ASSERT(std::is_sorted(hotelIds.begin(), hotelIds.end()), ());

  for (auto & hotelToResult : hotelToResults)
  {
    if (hotelToResult.m_cacheStatus != Cache::HotelStatus::Absent)
      continue;

    if (std::binary_search(hotelIds.cbegin(), hotelIds.cend(), hotelToResult.m_hotelId))
      cache.Insert(hotelToResult.m_hotelId, Cache::HotelStatus::Available);
    else
      cache.Insert(hotelToResult.m_hotelId, Cache::HotelStatus::Unavailable);
  }
}

void FillResults(HotelToResults && hotelToResults, std::vector<std::string> const & hotelIds,
                 availability::Cache & cache, search::Results & results)
{
  using availability::Cache;

  ASSERT(std::is_sorted(hotelIds.begin(), hotelIds.end()), ());

  for (auto & hotelToResult : hotelToResults)
  {
    switch (hotelToResult.m_cacheStatus)
    {
    case Cache::HotelStatus::Unavailable: continue;
    case Cache::HotelStatus::Available:
    {
      results.AddResult(std::move(hotelToResult.m_result));
      continue;
    }
    case Cache::HotelStatus::NotReady:
    {
      auto hotelStatus = cache.Get(hotelToResult.m_hotelId);

      if (hotelStatus == Cache::HotelStatus::Available)
        results.AddResult(std::move(hotelToResult.m_result));

      continue;
    }
    case Cache::HotelStatus::Absent:
    {
      if (std::binary_search(hotelIds.cbegin(), hotelIds.cend(), hotelToResult.m_hotelId))
        results.AddResult(std::move(hotelToResult.m_result));

      continue;
    }
    }
  }
}

void PrepareData(DataSource const & dataSource, search::Results const & results,
                 HotelToResults & hotelToResults, availability::Cache & cache,
                 booking::AvailabilityParams & p)
{
  std::vector<FeatureID> features;

  for (auto const & r : results)
  {
    if (!r.m_metadata.m_isSponsoredHotel || r.GetResultType() != search::Result::Type::Feature)
      continue;

    features.push_back(r.GetFeatureID());
    hotelToResults.emplace_back(r);
  }

  std::sort(features.begin(), features.end());

  MwmSet::MwmId mwmId;
  std::unique_ptr<FeaturesLoaderGuard> guard;
  for (auto const & featureId : features)
  {
    if (mwmId != featureId.m_mwmId)
    {
      guard = std::make_unique<FeaturesLoaderGuard>(dataSource, featureId.m_mwmId);
      mwmId = featureId.m_mwmId;
    }

    auto it = std::find_if(hotelToResults.begin(), hotelToResults.end(),
                           [&featureId](HotelToResult const & item)
                           {
                             return item.m_result.GetFeatureID() == featureId;
                           });
    ASSERT(it != hotelToResults.cend(), ());

    FeatureType ft;
    if (!guard->GetFeatureByIndex(featureId.m_index, ft))
    {
      hotelToResults.erase(it);
      LOG(LERROR, ("Feature can't be loaded:", featureId));
      continue;
    }

    auto const hotelId = ft.GetMetadata().Get(feature::Metadata::FMD_SPONSORED_ID);
    auto const status = cache.Get(hotelId);

    it->m_hotelId = hotelId;
    it->m_cacheStatus = status;

    if (status != availability::Cache::HotelStatus::Absent)
      continue;

    cache.Reserve(hotelId);
    p.m_hotelIds.push_back(std::move(hotelId));
  }
}
}  // namespace

namespace booking
{
namespace filter
{
AvailabilityFilter::AvailabilityFilter(Delegate const & d) : FilterBase(d) {}

void AvailabilityFilter::ApplyFilter(search::Results const & results,
                                     ParamsInternal const & filterParams)
{
  ASSERT(filterParams.m_apiParams, ());

  auto const & p = *filterParams.m_apiParams;
  auto const & cb = filterParams.m_callback;

  UpdateParams(p);

  m_apiParams.m_hotelIds.clear();

  HotelToResults hotelToResults;
  PrepareData(GetDelegate().GetDataSource(), results, hotelToResults, *m_cache, m_apiParams);

  if (m_apiParams.m_hotelIds.empty())
  {
    search::Results result;
    FillResults(std::move(hotelToResults), {} /* hotelIds */, *m_cache, result);
    cb(result);

    return;
  }

  auto availabilityCache = m_cache;

  auto const apiCallback =
    [cb, hotelToResults, availabilityCache](std::vector<std::string> hotelIds) mutable
    {
      GetPlatform().RunTask(Platform::Thread::File,
                            [cb, hotelToResults, availabilityCache, hotelIds]() mutable
                            {
                              search::Results results;
                              std::sort(hotelIds.begin(), hotelIds.end());
                              UpdateCache(hotelToResults, hotelIds, *availabilityCache);
                              FillResults(std::move(hotelToResults), hotelIds, *availabilityCache,
                                          results);
                              cb(results);
                            });
    };

  GetDelegate().GetApi().GetHotelAvailability(m_apiParams, apiCallback);
  m_cache->RemoveOutdated();
}

void AvailabilityFilter::UpdateParams(ParamsBase const & apiParams)
{
  if (m_apiParams.Equals(apiParams))
    return;

  m_apiParams.Set(apiParams);
  m_cache = std::make_shared<availability::Cache>();
}

void AvailabilityFilter::GetFeaturesFromCache(search::Results const & results,
                                              std::vector<FeatureID> & sortedResults)
{
  sortedResults.clear();

  std::vector<FeatureID> features;

  for (auto const & r : results)
  {
    if (!r.m_metadata.m_isSponsoredHotel || r.GetResultType() != search::Result::Type::Feature)
      continue;

    features.push_back(r.GetFeatureID());
  }

  std::sort(features.begin(), features.end());

  MwmSet::MwmId mwmId;
  std::unique_ptr<FeaturesLoaderGuard> guard;
  for (auto const & featureId : features)
  {
    if (mwmId != featureId.m_mwmId)
    {
      guard = std::make_unique<FeaturesLoaderGuard>(GetDelegate().GetDataSource(), featureId.m_mwmId);
      mwmId = featureId.m_mwmId;
    }

    FeatureType ft;
    if (!guard->GetFeatureByIndex(featureId.m_index, ft))
    {
      LOG(LERROR, ("Feature can't be loaded:", featureId));
      continue;
    }

    auto const & hotelId = ft.GetMetadata().Get(feature::Metadata::FMD_SPONSORED_ID);

    if (m_cache->Get(hotelId) == availability::Cache::HotelStatus::Available)
      sortedResults.push_back(featureId);
  }
}
}  // namespace booking
}  // namespace filter
