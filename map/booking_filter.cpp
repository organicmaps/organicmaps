#include "map/booking_filter.hpp"

#include "partners_api/booking_api.hpp"

#include "search/result.hpp"

#include "indexer/index.hpp"

#include "platform/platform.hpp"

#include <algorithm>
#include <utility>
#include <vector>

using namespace booking::filter;

namespace
{
struct HotelToResult
{
  HotelToResult(search::Result const & result) : m_result(result) {}

  search::Result m_result;
  std::string m_hotelId;
  availability::Cache::HotelStatus m_cacheStatus;
};

using HotelToResults = std::vector<HotelToResult>;

void FillAvailability(HotelToResult & hotelToResult, std::vector<std::string> const & hotelIds,
                      availability::Cache & cache, search::Results & results)
{
  using availability::Cache;

  if (hotelToResult.m_cacheStatus == Cache::HotelStatus::Unavailable)
    return;

  if (hotelToResult.m_cacheStatus == Cache::HotelStatus::Available)
  {
    results.AddResult(std::move(hotelToResult.m_result));
    return;
  }

  if (hotelToResult.m_cacheStatus == Cache::HotelStatus::NotReady)
  {
    auto hotelStatus = cache.Get(hotelToResult.m_hotelId);
    CHECK_NOT_EQUAL(hotelStatus, Cache::HotelStatus::Absent, ());
    CHECK_NOT_EQUAL(hotelStatus, Cache::HotelStatus::NotReady, ());

    if (hotelStatus == Cache::HotelStatus::Available)
      results.AddResult(std::move(hotelToResult.m_result));

    return;
  }

  if (hotelToResult.m_cacheStatus == Cache::HotelStatus::Absent)
  {
    if (std::binary_search(hotelIds.cbegin(), hotelIds.cend(), hotelToResult.m_hotelId))
    {
      results.AddResult(std::move(hotelToResult.m_result));
      cache.Insert(hotelToResult.m_hotelId, Cache::HotelStatus::Available);
    }
    else
    {
      cache.Insert(hotelToResult.m_hotelId, Cache::HotelStatus::Unavailable);
    }
  }
}

void PrepareData(Index const & index, search::Results const & results,
                 HotelToResults & hotelToResults, availability::Cache & cache,
                 booking::AvailabilityParams & p)
{
  std::vector<FeatureID> features;

  for (auto const & r : results)
  {
    if (!r.m_metadata.m_isSponsoredHotel ||
        r.GetResultType() != search::Result::ResultType::RESULT_FEATURE)
    {
      continue;
    }

    features.push_back(r.GetFeatureID());
    hotelToResults.emplace_back(r);
  }

  std::sort(features.begin(), features.end());

  MwmSet::MwmId currentMwmId;
  std::unique_ptr<Index::FeaturesLoaderGuard> guard;
  for (auto const & featureId : features)
  {
    if (currentMwmId != featureId.m_mwmId)
    {
      guard = my::make_unique<Index::FeaturesLoaderGuard>(index, featureId.m_mwmId);
      currentMwmId = featureId.m_mwmId;
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

    std::string hotelId = ft.GetMetadata().Get(feature::Metadata::FMD_SPONSORED_ID);
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
Filter::Filter(Index const & index, booking::Api const & api) : m_index(index), m_api(api) {}

void Filter::Availability(search::Results const & results,
                          availability::internal::Params && params)
{
  GetPlatform().RunTask(Platform::Thread::File, [this, results, params]()
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto const & cb = params.m_callback;

    ASSERT(params.m_params.m_hotelIds.empty(), ());

    if (m_currentParams != params.m_params)
    {
      m_currentParams = std::move(params.m_params);
      m_availabilityCache.Drop();
      ++m_cacheDropCounter;
    }

    HotelToResults hotelToResults;

    PrepareData(m_index, results, hotelToResults, m_availabilityCache, m_currentParams);

    auto const dropCounter = m_cacheDropCounter;

    // This callback will be called on network thread.
    auto const apiCallback = [this, dropCounter, cb, hotelToResults](std::vector<std::string> hotelIds) mutable
    {
      search::Results result;
      {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_cacheDropCounter == dropCounter)
        {
          std::sort(hotelIds.begin(), hotelIds.end());

          for (auto & item : hotelToResults)
          {
            FillAvailability(item, hotelIds, m_availabilityCache, result);
          }
        }
      }

      cb(result);
    };

    m_api.GetHotelAvailability(m_currentParams, apiCallback);
    m_availabilityCache.RemoveOutdated();
  });
}
}  // namespace filter
}  // namespace booking
