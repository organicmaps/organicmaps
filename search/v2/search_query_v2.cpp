#include "search/v2/search_query_v2.hpp"

#include "search/dummy_rank_table.hpp"

#include "indexer/rank_table.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "base/macros.hpp"

namespace search
{
namespace v2
{
SearchQueryV2::SearchQueryV2(Index & index, CategoriesHolder const & categories,
                             vector<Suggest> const & suggests,
                             storage::CountryInfoGetter const & infoGetter)
  : Query(index, categories, suggests, infoGetter), m_geocoder(index, infoGetter)
{
  m_keepHouseNumberInQuery = true;
}

void SearchQueryV2::Reset()
{
  Query::Reset();
  m_geocoder.Reset();
}

void SearchQueryV2::Cancel()
{
  Query::Cancel();
  m_geocoder.Cancel();
}

void SearchQueryV2::Search(Results & res, size_t resCount)
{
  if (m_tokens.empty())
    SuggestStrings(res);

  Geocoder::Params params;
  InitParams(false /* localitySearch */, params);
  params.m_mode = m_mode;

  params.m_pivot = GetPivotRect();
  params.m_accuratePivotCenter = GetPivotPoint();
  params.m_maxNumResults = max(resCount, kPreResultsCount);
  m_geocoder.SetParams(params);

  Geocoder::TResultList results;
  m_geocoder.GoEverywhere(results);
  AddPreResults1(results);

  FlushResults(params, res, false /* allMWMs */, resCount, false /* oldHouseSearch */);
}

void SearchQueryV2::SearchViewportPoints(Results & res)
{
  Geocoder::Params params;
  InitParams(false /* localitySearch */, params);
  params.m_pivot = m_viewport[CURRENT_V];
  params.m_accuratePivotCenter = params.m_pivot.Center();
  params.m_maxNumResults = kPreResultsCount;
  m_geocoder.SetParams(params);

  Geocoder::TResultList results;
  m_geocoder.GoInViewport(results);
  AddPreResults1(results);

  FlushViewportResults(params, res, false /* oldHouseSearch */);
}

void SearchQueryV2::ClearCaches()
{
  Query::ClearCaches();
  m_geocoder.ClearCaches();
}

void SearchQueryV2::AddPreResults1(Geocoder::TResultList & results)
{
  for (auto const & result : results)
  {
    auto const & id = result.first;
    auto const & info = result.second;
    AddPreResult1(id.m_mwmId, id.m_index, info.m_distanceToPivot /* priority */, info);
  }
}
}  // namespace v2
}  // namespace search
