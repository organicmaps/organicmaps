#include "map/booking_filter.hpp"

#include "partners_api/booking_api.hpp"

#include "search/result.hpp"

#include "indexer/index.hpp"

#include <algorithm>
#include <utility>
#include <vector>

using namespace booking::filter;

namespace
{
struct HotelToResult
{
  HotelToResult(std::string const & id, search::Result const & result,
                availability::Cache::HotelStatus status)
    : m_hotelId(id), m_result(result), m_cacheStatus(status)
  {
  }

  std::string m_hotelId;
  search::Result m_result;
  availability::Cache::HotelStatus m_cacheStatus;
};

void FillAvailability(HotelToResult & hotelToResult, std::vector<std::string> const & hotelIds,
                      availability::Cache & cache, search::Results & results)
{
  using availability::Cache;

  if (hotelToResult.m_cacheStatus == Cache::HotelStatus::UnAvailable)
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
      cache.Insert(hotelToResult.m_hotelId, Cache::HotelStatus::UnAvailable);
    }
  }
}
}  // namespace

namespace booking
{
namespace filter
{
Filter::Filter(Index const & index, booking::Api const & api) : m_index(index), m_api(api) {}

void Filter::Availability(search::Results const & results,
                          availability::internal::Params const & params) const
{
  auto & p = params.m_params;
  auto const & cb = params.m_callback;

  ASSERT(p.m_hotelIds.empty(), ());

  std::vector<HotelToResult> hotelToResults;
  // Fill hotel ids.
  for (auto const & r : results)
  {
    if (!r.m_metadata.m_isSponsoredHotel ||
        r.GetResultType() != search::Result::ResultType::RESULT_FEATURE)
      continue;

    auto const & id = r.GetFeatureID();
    Index::FeaturesLoaderGuard const guard(m_index, id.m_mwmId);
    FeatureType ft;
    if (!guard.GetFeatureByIndex(id.m_index, ft))
    {
      LOG(LERROR, ("Feature can't be loaded:", id));
      continue;
    }

    std::string hotelId = ft.GetMetadata().Get(feature::Metadata::FMD_SPONSORED_ID);
    auto const status = m_availabilityCache.Get(hotelId);
    hotelToResults.emplace_back(hotelId, r, status);

    if (status != availability::Cache::HotelStatus::Absent)
      continue;

    m_availabilityCache.Reserve(hotelId);
    p.m_hotelIds.push_back(std::move(hotelId));
  }

  auto const apiCallback = [this, cb, hotelToResults](std::vector<std::string> hotelIds) mutable
  {
    std::sort(hotelIds.begin(), hotelIds.end());

    search::Results result;
    for (auto & item : hotelToResults)
    {
      FillAvailability(item, hotelIds, m_availabilityCache, result);
    }

    cb(result);
  };

  m_api.GetHotelAvailability(p, apiCallback);
}
}  // namespace filter
}  // namespace booking
