#include "map/discovery/discovery_search.hpp"

#include "search/intermediate_result.hpp"
#include "search/utils.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature_algo.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <set>
#include <utility>

namespace
{
search::Result MakeResultFromFeatureType(FeatureType const & ft)
{
  std::string name;
  ft.GetName(StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm()), name);

  feature::TypesHolder holder(ft);
  holder.SortBySpec();
  CategoriesHolder const & categories = GetDefaultCategories();
  auto const readableType = categories.GetReadableFeatureType(
      holder.GetBestType(), categories.MapLocaleToInteger(languages::GetCurrentOrig()));

  search::Result::Metadata metadata;
  search::ProcessMetadata(ft, metadata);

  return {ft.GetID(), feature::GetCenter(ft), name, "", readableType, holder.GetBestType(), metadata};
}

FeatureType MakeFeatureTypeWithCachedGuard(DataSource const & dataSource, MwmSet::MwmId & mwmId,
                                           std::unique_ptr<FeaturesLoaderGuard> & guard,
                                           FeatureID const & id)
{
  if (mwmId != id.m_mwmId)
  {
    guard = my::make_unique<FeaturesLoaderGuard>(dataSource, id.m_mwmId);
    mwmId = id.m_mwmId;
  }

  FeatureType ft;
  if (!guard->GetFeatureByIndex(id.m_index, ft))
  {
    LOG(LERROR, ("Feature can't be loaded:", id));
    return {};
  }

  return ft;
}

class GreaterRating
{
public:
  bool operator()(FeatureType const & lhs, FeatureType const & rhs) const
  {
    double constexpr kPenaltyRating = -1.0;
    double lhsRating = kPenaltyRating;
    double rhsRating = kPenaltyRating;

    if (!strings::to_double(lhs.GetMetadata().Get(feature::Metadata::EType::FMD_RATING), lhsRating))
      lhsRating = kPenaltyRating;

    if (!strings::to_double(rhs.GetMetadata().Get(feature::Metadata::EType::FMD_RATING), rhsRating))
      rhsRating = kPenaltyRating;

    return lhsRating > rhsRating;
  }
};
}  // namespace
namespace discovery
{
SearchBase::SearchBase(DataSource const & dataSource, DiscoverySearchParams const & params,
                       search::ProductInfo::Delegate const & productInfoDelegate)
  : m_dataSource(dataSource)
  , m_params(params)
  , m_productInfoDelegate(productInfoDelegate)
{
  CHECK(params.m_onResults, ());
  CHECK(!params.m_query.empty(), ());
  CHECK_GREATER(params.m_itemsCount, 0, ());
}

void SearchBase::Search()
{
  MwmSet::MwmId currentMwmId;
  MwmSet::MwmHandle currentMwmHandle;
  search::ForEachOfTypesInRect(m_dataSource,
                               search::GetCategoryTypes(m_params.m_query, "en", GetDefaultCategories()),
                               m_params.m_viewport,
                               [&](FeatureID const & id)
                               {
                                 if (currentMwmId != id.m_mwmId)
                                 {
                                   currentMwmId = id.m_mwmId;
                                   OnMwmChanged(m_dataSource.GetMwmHandleById(id.m_mwmId));
                                 }

                                 ProcessFeatureId(id);
                               });

  ProcessAccumulated();

  if (m_params.m_onResults)
    m_params.m_onResults(m_results, m_productInfo);
}

search::Results const & SearchBase::GetResults() const
{
  return m_results;
}

std::vector<search::ProductInfo> const & SearchBase::GetProductInfo() const
{
  return m_productInfo;
}

DataSource const & SearchBase::GetDataSource() const
{
  return m_dataSource;
}

DiscoverySearchParams const & SearchBase::GetParams() const
{
  return m_params;
}

void SearchBase::AppendResult(search::Result && result)
{
  m_productInfo.emplace_back(m_productInfoDelegate.GetProductInfo(result));
  m_results.AddResultNoChecks(std::move(result));
}

void SearchBase::OnMwmChanged(MwmSet::MwmHandle const & handle)
{
}

SearchHotels::SearchHotels(DataSource const & dataSource, DiscoverySearchParams const & params,
                           search::ProductInfo::Delegate const & productInfoDelegate)
  : SearchBase(dataSource, params, productInfoDelegate)
{
}

void SearchHotels::ProcessFeatureId(FeatureID const & id)
{
  m_featureIds.emplace_back(id);
}

void SearchHotels::ProcessAccumulated()
{
  ASSERT(std::is_sorted(m_featureIds.cbegin(), m_featureIds.cend()), ());

  MwmSet::MwmId mwmId;
  std::unique_ptr<FeaturesLoaderGuard> guard;

  auto const makeFeatureType = [this, &guard, &mwmId](FeatureID const & id)
  {
    return MakeFeatureTypeWithCachedGuard(GetDataSource(), mwmId, guard, id);
  };

  std::multiset<FeatureType, GreaterRating> sortedByRating;

  for (auto const & id : m_featureIds)
  {
    sortedByRating.emplace(makeFeatureType(id));
  }

  for (auto const & ft : sortedByRating)
  {
    auto result = MakeResultFromFeatureType(ft);
    AppendResult(std::move(result));

    if (GetResults().GetCount() >= GetParams().m_itemsCount)
      break;
  }
}

SearchPopularPlaces::SearchPopularPlaces(DataSource const & dataSource,
                                         DiscoverySearchParams const & params,
                                         search::ProductInfo::Delegate const & productInfoDelegate)
  : SearchBase(dataSource, params, productInfoDelegate)
{
}

void SearchPopularPlaces::OnMwmChanged(MwmSet::MwmHandle const & handle)
{
  m_popularityRanks.reset();
  if (handle.IsAlive())
  {
    m_popularityRanks =
        search::RankTable::Load(handle.GetValue<MwmValue>()->m_cont, POPULARITY_RANKS_FILE_TAG);
  }
}

void SearchPopularPlaces::ProcessFeatureId(FeatureID const & id)
{
  uint8_t popularity = 0;

  if (m_popularityRanks)
    popularity = m_popularityRanks->Get(id.m_index);

  if (popularity != 0 || m_accumulatedResults.size() < GetParams().m_itemsCount)
  {
    m_accumulatedResults.emplace(popularity, id);
  }
}

void SearchPopularPlaces::ProcessAccumulated()
{
  MwmSet::MwmId mwmId;
  std::unique_ptr<FeaturesLoaderGuard> guard;

  auto const makeFeatureType = [this, &guard, &mwmId](FeatureID const & id)
  {
    return MakeFeatureTypeWithCachedGuard(GetDataSource(), mwmId, guard, id);
  };

  auto const appendResult = [this](uint8_t popularity, FeatureType const & ft)
  {
    auto result = MakeResultFromFeatureType(ft);

    search::RankingInfo rankingInfo;
    rankingInfo.m_popularity = popularity;
    result.SetRankingInfo(std::move(rankingInfo));

    AppendResult(std::move(result));
  };

  auto const itemsCount = GetParams().m_itemsCount;

  std::vector<FeatureID> featuresWithoutNames;
  for (auto const & item : m_accumulatedResults)
  {
    if (GetResults().GetCount() >= itemsCount)
      break;

    auto const popularity = item.first;
    auto const ft = makeFeatureType(item.second);

    if (popularity > 0)
    {
      appendResult(popularity, ft);
      continue;
    }

    if (ft.HasName())
    {
      appendResult(popularity, ft);
      continue;
    }

    if (featuresWithoutNames.size() < (itemsCount - GetResults().GetCount()))
      featuresWithoutNames.emplace_back(item.second);
  }

  // Append unnamed results if needed.
  if (GetResults().GetCount() < itemsCount)
  {
    for (auto const & featureId : featuresWithoutNames)
    {
      appendResult(0, makeFeatureType(featureId));

      if (GetResults().GetCount() >= itemsCount)
        break;
    }
  }
}

void ProcessSearchIntent(std::shared_ptr<SearchBase> intent)
{
  if (!intent)
    return;

  GetPlatform().RunTask(Platform::Thread::File, [intent]() { intent->Search(); });
}
}  // namespace discovery
