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

void SearchQueryV2::Search(Results & results, size_t limit)
{
  if (m_tokens.empty())
    SuggestStrings(results);

  Geocoder::Params params;
  InitParams(false /* localitySearch */, params);
  params.m_mode = m_mode;

  params.m_pivot = GetPivotRect();
  params.m_accuratePivotCenter = GetPivotPoint();
  m_geocoder.SetParams(params);

  m_geocoder.GoEverywhere(m_preRanker);

  FlushResults(params, results, false /* allMWMs */, limit, false /* oldHouseSearch */);
}

void SearchQueryV2::SearchViewportPoints(Results & results)
{
  Geocoder::Params params;
  InitParams(false /* localitySearch */, params);
  params.m_pivot = m_viewport[CURRENT_V];
  params.m_accuratePivotCenter = params.m_pivot.Center();
  m_geocoder.SetParams(params);

  m_geocoder.GoInViewport(m_preRanker);

  FlushViewportResults(params, results, false /* oldHouseSearch */);
}

void SearchQueryV2::ClearCaches()
{
  Query::ClearCaches();
  m_geocoder.ClearCaches();
}
}  // namespace v2
}  // namespace search
