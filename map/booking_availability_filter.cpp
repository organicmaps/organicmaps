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
  HotelInfo(std::string const & hotelId, availability::Cache::Info && info, T const & mappedValue)
    : m_hotelId(hotelId), m_info(std::move(info)), m_mappedValue(mappedValue)
  {
  }

  void SetHotelId(std::string const & hotelId) { m_hotelId = hotelId; }
  void SetInfo(availability::Cache::Info && info) { m_info = std::move(info); }

  std::string const & GetHotelId() const { return m_hotelId; }
  availability::Cache::HotelStatus GetStatus() const { return m_info.m_status; }

  std::optional<booking::Extras> const & GetExtras() const { return m_info.m_extras; }
  std::optional<booking::Extras> & GetExtras() { return m_info.m_extras; }

  T const & GetMappedValue() const { return m_mappedValue; }
  T & GetMappedValue() { return m_mappedValue; }

private:
  std::string m_hotelId;
  availability::Cache::Info m_info;
  T m_mappedValue;
};

template <typename T>
using HotelsMapping = std::vector<HotelInfo<T>>;

using HotelToResult = HotelInfo<search::Result>;
using HotelToFeatureId = HotelInfo<FeatureID>;
using HotelToResults = HotelsMapping<search::Result>;
using HotelToFeatureIds = HotelsMapping<FeatureID>;

bool IsConformToFilter(search::Result const & r)
{
  return r.m_details.m_isSponsoredHotel && r.GetResultType() == search::Result::Type::Feature;
}

template <typename T>
void UpdateCache(HotelsMapping<T> const & hotelsMapping, booking::HotelsWithExtras const & hotels,
                 availability::Cache & cache)
{
  using availability::Cache;

  cache.ReserveAdditional(hotelsMapping.size());

  for (auto & hotel : hotelsMapping)
  {
    if (hotel.GetStatus() != Cache::HotelStatus::Absent)
      continue;
    auto const it = hotels.find(hotel.GetHotelId());
    if (it != hotels.cend())
      cache.InsertAvailable(hotel.GetHotelId(), {it->second.m_price, it->second.m_currency});
    else
      cache.InsertUnavailable(hotel.GetHotelId());
  }
}

template <typename T, typename PassedInserter, typename FilteredOutInserter>
void FillResults(HotelsMapping<T> && hotelsMapping, booking::HotelsWithExtras && hotels,
                 availability::Cache & cache, PassedInserter const & passedInserter,
                 FilteredOutInserter const & filteredOutInserter)
{
  using availability::Cache;

  for (auto & hotel : hotelsMapping)
  {
    switch (hotel.GetStatus())
    {
    case Cache::HotelStatus::Unavailable:
    {
      filteredOutInserter(std::move(hotel.GetMappedValue()));
      continue;
    }
    case Cache::HotelStatus::Available:
    {
      passedInserter(std::move(hotel.GetMappedValue()), std::move(*hotel.GetExtras()));
      continue;
    }
    case Cache::HotelStatus::NotReady:
    {
      auto info = cache.Get(hotel.GetHotelId());

      if (info.m_status == Cache::HotelStatus::Available)
        passedInserter(std::move(hotel.GetMappedValue()), std::move(*info.m_extras));
      else if (info.m_status == Cache::HotelStatus::Unavailable)
        filteredOutInserter(std::move(hotel.GetMappedValue()));

      continue;
    }
    case Cache::HotelStatus::Absent:
    {
      auto const it = hotels.find(hotel.GetHotelId());
      if (it != hotels.cend())
        passedInserter(std::move(hotel.GetMappedValue()), std::move(it->second));
      else
        filteredOutInserter(std::move(hotel.GetMappedValue()));

      continue;
    }
    }
  }
}

void FillResults(HotelToResults && hotelToResults, booking::HotelsWithExtras && hotels,
                 availability::Cache & cache, ResultInternal<search::Results> & results)
{
  auto const passedInserter = [&results](search::Result && passed, booking::Extras && extra)
  {
    results.m_passedFilter.AddResultNoChecks(std::move(passed));
    results.m_extras.emplace_back(std::move(extra));
  };

  auto const filteredOutInserter = [&results](search::Result && filteredOut)
  {
    results.m_filteredOut.AddResultNoChecks(std::move(filteredOut));
  };

  FillResults(std::move(hotelToResults), std::move(hotels), cache, passedInserter, filteredOutInserter);
}

void FillResults(HotelToFeatureIds && hotelToFeatureIds, booking::HotelsWithExtras && hotels,
                 availability::Cache & cache, ResultInternal<std::vector<FeatureID>> & results)
{
  auto const passedInserter = [&results](FeatureID && passed, booking::Extras && extra)
  {
    results.m_passedFilter.emplace_back(std::move(passed));
    results.m_extras.emplace_back(std::move(extra));
  };

  auto const filteredOutInserter = [&results](FeatureID && filteredOut)
  {
    results.m_filteredOut.emplace_back(std::move(filteredOut));
  };

  FillResults(std::move(hotelToFeatureIds), std::move(hotels), cache, passedInserter, filteredOutInserter);
}

void PrepareData(DataSource const & dataSource, search::Results const & results,
                 HotelToResults & hotelToResults, availability::Cache & cache,
                 booking::AvailabilityParams & p)
{
  std::vector<FeatureID> features;

  for (auto const & r : results)
  {
    if (!IsConformToFilter(r))
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

    auto hotelId = ft->GetMetadata(feature::Metadata::FMD_SPONSORED_ID);
    auto info = cache.Get(hotelId);
    auto const status = info.m_status;

    it->SetHotelId(hotelId);
    it->SetInfo(std::move(info));

    if (status != availability::Cache::HotelStatus::Absent)
      continue;

    cache.InsertNotReady(hotelId);
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
  for (auto const & featureId : featureIds)
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

    auto const hotelId = ft->GetMetadata(feature::Metadata::FMD_SPONSORED_ID);
    auto info = cache.Get(hotelId);
    auto const status = info.m_status;

    hotelToFeatures.emplace_back(hotelId, std::move(info), featureId);

    if (status != availability::Cache::HotelStatus::Absent)
      continue;

    cache.InsertNotReady(hotelId);
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
  ASSERT(std::is_sorted(featureIds.cbegin(), featureIds.cend()), ());

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
    booking::filter::ResultInternal<Source> result;
    FillResults(std::move(hotelsToSourceValue), {} /* hotelIds */, *m_cache, result);
    cb(std::move(result));

    return;
  }

  auto const apiCallback =
    [cb, cache = m_cache, hotelToValue = std::move(hotelsToSourceValue)] (booking::HotelsWithExtras hotels) mutable
  {
    GetPlatform().RunTask(
      Platform::Thread::File,
      [cb, cache, hotelToValue = std::move(hotelToValue), hotels = std::move(hotels)]() mutable
    {
      UpdateCache(hotelToValue, hotels, *cache);

      booking::filter::ResultInternal<Source> updatedResult;
      FillResults(std::move(hotelToValue), std::move(hotels), *cache, updatedResult);
      cb(std::move(updatedResult));
    });
  };

  GetDelegate().GetApi().GetHotelAvailability(m_apiParams, apiCallback);
  m_cache->RemoveOutdated();
}

void AvailabilityFilter::GetFeaturesFromCache(search::Results const & results,
                                              std::vector<FeatureID> & sortedResults,
                                              std::vector<Extras> & extras,
                                              std::vector<FeatureID> & filteredOut)
{
  sortedResults.clear();

  std::vector<FeatureID> features;

  for (auto const & r : results)
  {
    if (!IsConformToFilter(r))
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

    auto const & hotelId = ft->GetMetadata(feature::Metadata::FMD_SPONSORED_ID);

    auto info = m_cache->Get(hotelId);
    if (info.m_status == availability::Cache::HotelStatus::Available)
    {
      sortedResults.push_back(featureId);
      extras.emplace_back(std::move(*info.m_extras));
    }
    else if (info.m_status == availability::Cache::HotelStatus::Unavailable)
    {
      filteredOut.push_back(featureId);
    }
  }
}
}  // namespace booking
}  // namespace filter
