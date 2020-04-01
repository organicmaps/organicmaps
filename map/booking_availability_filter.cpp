#include "map/booking_availability_filter.hpp"

#include "search/result.hpp"

#include "partners_api/booking_api.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/ftypes_sponsored.hpp"

#include "platform/platform.hpp"

#include "base/macros.hpp"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

using namespace booking::filter;

namespace
{
auto constexpr kMaxCountInRequest = booking::RawApi::GetMaxHotelsInAvailabilityRequest();

template <typename T>
class HotelInfo
{
public:
  explicit HotelInfo(T const & requestedVia) : m_mappedValue(requestedVia) {}
  HotelInfo(std::string const & hotelId, availability::Cache::HotelStatus const & cacheStatus,
            T const & mappedValue)
    : m_hotelId(hotelId), m_cacheStatus(cacheStatus), m_mappedValue(mappedValue)
  {
  }

  void SetHotelId(std::string const & hotelId) { m_hotelId = hotelId; }
  void SetStatus(availability::Cache::HotelStatus cacheStatus) { m_cacheStatus = cacheStatus; }

  std::string const & GetHotelId() const { return m_hotelId; }
  availability::Cache::HotelStatus GetStatus() const { return m_cacheStatus; }

  T const & GetMappedValue() const { return m_mappedValue; }
  T & GetMappedValue() { return m_mappedValue; }

private:
  std::string m_hotelId;
  availability::Cache::HotelStatus m_cacheStatus = availability::Cache::HotelStatus::Absent;
  T m_mappedValue;
};

template <typename T>
using HotelsMapping = std::vector<HotelInfo<T>>;

using HotelToResult = HotelInfo<search::Result>;
using HotelToFeatureId = HotelInfo<FeatureID>;
using HotelToResults = HotelsMapping<search::Result>;
using HotelToFeatureIds = HotelsMapping<FeatureID>;

template <typename T>
void UpdateCache(HotelsMapping<T> const & hotelsMapping, std::vector<std::string> const & hotelIds,
                 availability::Cache & cache)
{
  using availability::Cache;

  ASSERT(std::is_sorted(hotelIds.begin(), hotelIds.end()), ());

  for (auto & hotel : hotelsMapping)
  {
    if (hotel.GetStatus() != Cache::HotelStatus::Absent)
      continue;

    if (std::binary_search(hotelIds.cbegin(), hotelIds.cend(), hotel.GetHotelId()))
      cache.Insert(hotel.GetHotelId(), Cache::HotelStatus::Available);
    else
      cache.Insert(hotel.GetHotelId(), Cache::HotelStatus::Unavailable);
  }
}

template <typename T, typename Inserter>
void FillResults(HotelsMapping<T> && hotelsMapping, std::vector<std::string> const & hotelIds,
                 availability::Cache & cache, Inserter const & inserter)
{
  using availability::Cache;

  ASSERT(std::is_sorted(hotelIds.begin(), hotelIds.end()), ());

  for (auto & hotel : hotelsMapping)
  {
    switch (hotel.GetStatus())
    {
    case Cache::HotelStatus::Unavailable: continue;
    case Cache::HotelStatus::Available:
    {
      inserter(std::move(hotel.GetMappedValue()));
      continue;
    }
    case Cache::HotelStatus::NotReady:
    {
      auto hotelStatus = cache.Get(hotel.GetHotelId());

      if (hotelStatus == Cache::HotelStatus::Available)
        inserter(std::move(hotel.GetMappedValue()));

      continue;
    }
    case Cache::HotelStatus::Absent:
    {
      if (std::binary_search(hotelIds.cbegin(), hotelIds.cend(), hotel.GetHotelId()))
        inserter(std::move(hotel.GetMappedValue()));

      continue;
    }
    }
  }
}

void FillResults(HotelToResults && hotelToResults, std::vector<std::string> const & hotelIds,
                 availability::Cache & cache, search::Results & results)
{
  auto const inserter = [&results](search::Result && result)
  {
    results.AddResult(std::move(result));
  };

  FillResults(std::move(hotelToResults), hotelIds, cache, inserter);
}

void FillResults(HotelToFeatureIds && hotelToFeatureIds, std::vector<std::string> const & hotelIds,
                 availability::Cache & cache, std::vector<FeatureID> & results)
{
  auto const inserter = [&results](FeatureID && result)
  {
    results.emplace_back(std::move(result));
  };

  FillResults(std::move(hotelToFeatureIds), hotelIds, cache, inserter);
}

void PrepareData(DataSource const & dataSource, search::Results const & results,
                 HotelToResults & hotelToResults, availability::Cache & cache,
                 booking::AvailabilityParams & p)
{
  std::vector<FeatureID> features;

  for (auto const & r : results)
  {
    if (!r.m_details.m_isSponsoredHotel || r.GetResultType() != search::Result::Type::Feature)
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
                             return item.GetMappedValue().GetFeatureID() == featureId;
                           });
    ASSERT(it != hotelToResults.cend(), ());

    auto ft = guard->GetFeatureByIndex(featureId.m_index);
    if (!ft)
    {
      hotelToResults.erase(it);
      LOG(LERROR, ("Feature can't be loaded:", featureId));
      continue;
    }

    if (!ftypes::IsBookingChecker::Instance()(*ft))
      continue;

    auto const hotelId = ft->GetMetadata().Get(feature::Metadata::FMD_SPONSORED_ID);
    auto const status = cache.Get(hotelId);

    it->SetHotelId(hotelId);
    it->SetStatus(status);

    if (status != availability::Cache::HotelStatus::Absent)
      continue;

    cache.Reserve(hotelId);
    p.m_hotelIds.push_back(std::move(hotelId));
  }
}

void PrepareData(DataSource const & dataSource, std::vector<FeatureID> const & featureIds,
                 HotelToFeatureIds & hotelToFeatures, availability::Cache & cache,
                 booking::AvailabilityParams & p)
{
  ASSERT(std::is_sorted(featureIds.begin(), featureIds.end()), ());

  MwmSet::MwmId mwmId;
  std::unique_ptr<FeaturesLoaderGuard> guard;
  for (auto const featureId : featureIds)
  {
    if (mwmId != featureId.m_mwmId)
    {
      guard = std::make_unique<FeaturesLoaderGuard>(dataSource, featureId.m_mwmId);
      mwmId = featureId.m_mwmId;
    }

    auto ft = guard->GetFeatureByIndex(featureId.m_index);
    if (!ft)
    {
      LOG(LERROR, ("Feature can't be loaded:", featureId));
      continue;
    }

    if (!ftypes::IsBookingChecker::Instance()(*ft))
      continue;

    auto const hotelId = ft->GetMetadata().Get(feature::Metadata::FMD_SPONSORED_ID);
    auto const status = cache.Get(hotelId);

    hotelToFeatures.emplace_back(hotelId, status, featureId);

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
  ApplyFilterInternal<search::Result>(results, filterParams);
}

void AvailabilityFilter::ApplyFilter(std::vector<FeatureID> const & featureIds,
                                     ParamsRawInternal const & filterParams)
{
  ApplyFilterInternal<FeatureID>(featureIds, filterParams);
}

void AvailabilityFilter::UpdateParams(ParamsBase const & apiParams)
{
  if (m_apiParams.Equals(apiParams))
    return;

  m_apiParams.Set(apiParams);
  m_cache = std::make_shared<availability::Cache>();
}

template <typename SourceValue, typename Source, typename Parameters>
void AvailabilityFilter::ApplyFilterInternal(Source const & source, Parameters const & filterParams)
{
  ASSERT(filterParams.m_apiParams, ());

  auto const & cb = filterParams.m_callback;

  UpdateParams(*filterParams.m_apiParams);

  m_apiParams.m_hotelIds.clear();

  HotelsMapping<SourceValue> hotelsToSourceValue;
  PrepareData(GetDelegate().GetDataSource(), source, hotelsToSourceValue, *m_cache, m_apiParams);

  UNUSED_VALUE(kMaxCountInRequest);
  ASSERT_LESS_OR_EQUAL(m_apiParams.m_hotelIds.size(), kMaxCountInRequest, ());

  if (m_apiParams.m_hotelIds.empty())
  {
    Source result;
    FillResults(std::move(hotelsToSourceValue), {} /* hotelIds */, *m_cache, result);
    cb(result);

    return;
  }

  auto const apiCallback =
    [cb, cache = m_cache, hotelToValue = std::move(hotelsToSourceValue)]
    (std::vector<std::string> hotelIds) mutable
  {
    GetPlatform().RunTask(
      Platform::Thread::File,
      [cb, cache, hotelToValue = std::move(hotelToValue), hotelIds = std::move(hotelIds)]() mutable
    {
      Source updatedResults;
      std::sort(hotelIds.begin(), hotelIds.end());
      UpdateCache(hotelToValue, hotelIds, *cache);
      FillResults(std::move(hotelToValue), hotelIds, *cache, updatedResults);
      cb(updatedResults);
    });
  };

  GetDelegate().GetApi().GetHotelAvailability(m_apiParams, apiCallback);
  m_cache->RemoveOutdated();
}

void AvailabilityFilter::GetFeaturesFromCache(search::Results const & results,
                                              std::vector<FeatureID> & sortedResults)
{
  sortedResults.clear();

  std::vector<FeatureID> features;

  for (auto const & r : results)
  {
    if (!r.m_details.m_isSponsoredHotel || r.GetResultType() != search::Result::Type::Feature)
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

    auto ft = guard->GetFeatureByIndex(featureId.m_index);
    if (!ft)
    {
      LOG(LERROR, ("Feature can't be loaded:", featureId));
      continue;
    }

    auto const & hotelId = ft->GetMetadata().Get(feature::Metadata::FMD_SPONSORED_ID);

    if (m_cache->Get(hotelId) == availability::Cache::HotelStatus::Available)
      sortedResults.push_back(featureId);
  }
}
}  // namespace booking
}  // namespace filter
