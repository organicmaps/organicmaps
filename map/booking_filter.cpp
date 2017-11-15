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
  explicit HotelToResult(search::Result const & result) : m_result(result) {}

  search::Result m_result;
  std::string m_hotelId;
  availability::Cache::HotelStatus m_cacheStatus;
};

using HotelToResults = std::vector<HotelToResult>;

void FillResults(HotelToResults && hotelToResults, std::vector<std::string> const & hotelIds,
                 availability::Cache & cache, search::Results & results)
{
  using availability::Cache;

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
        CHECK_NOT_EQUAL(hotelStatus, Cache::HotelStatus::Absent, ());
        CHECK_NOT_EQUAL(hotelStatus, Cache::HotelStatus::NotReady, ());

        if (hotelStatus == Cache::HotelStatus::Available)
          results.AddResult(std::move(hotelToResult.m_result));

        continue;
      }
      case Cache::HotelStatus::Absent:
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

        continue;
      }
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
Filter::Filter(Index const & index, booking::Api const & api) : m_index(index), m_api(api) {}

void Filter::FilterAvailability(search::Results const & results,
                                availability::internal::Params && params)
{
  GetPlatform().RunTask(Platform::Thread::File, [this, results, params]()
  {
    auto const & cb = params.m_callback;

    ASSERT(params.m_params.m_hotelIds.empty(), ());

    if (m_currentParams != params.m_params)
    {
      m_currentParams = std::move(params.m_params);
      m_availabilityCache.Clear();
    }

    HotelToResults hotelToResults;
    PrepareData(m_index, results, hotelToResults, m_availabilityCache, m_currentParams);

    if (m_currentParams.m_hotelIds.empty())
    {
      search::Results result;
      FillResults(std::move(hotelToResults), {}, m_availabilityCache, result);
      cb(result);

      return;
    }

    auto const paramsCopy = m_currentParams;
    auto const apiCallback =
      [this, paramsCopy, cb, hotelToResults](std::vector<std::string> hotelIds) mutable
    {
      GetPlatform().RunTask(Platform::Thread::File,
                            [this, paramsCopy, cb, hotelToResults, hotelIds]() mutable
      {
        search::Results result;
        if (paramsCopy == m_currentParams)
        {
          std::sort(hotelIds.begin(), hotelIds.end());
          FillResults(std::move(hotelToResults), hotelIds, m_availabilityCache, result);
        }

        cb(result);
      });
    };

    m_api.GetHotelAvailability(m_currentParams, apiCallback);
    m_availabilityCache.RemoveOutdated();
  });
}
}  // namespace filter
}  // namespace booking
