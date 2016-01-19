#include "search/v2/search_query_v2.hpp"

#include "search/dummy_rank_table.hpp"

#include "indexer/rank_table.hpp"

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
  params.m_viewport = m_viewport[CURRENT_V];
  params.m_position = m_position;
  params.m_maxNumResults = max(resCount, kPreResultsCount);
  m_geocoder.SetParams(params);

  vector<FeatureID> results;
  m_geocoder.Go(results);
  AddPreResults1(results);

  FlushResults(res, false /* allMWMs */, resCount, false /* oldHouseSearch */);
}

void SearchQueryV2::SearchViewportPoints(Results & res)
{
  Geocoder::Params params;
  InitParams(false /* localitySearch */, params);
  params.m_viewport = m_viewport[CURRENT_V];
  params.m_maxNumResults = kPreResultsCount;
  m_geocoder.SetParams(params);

  vector<FeatureID> results;
  m_geocoder.GoInViewport(results);
  AddPreResults1(results);

  FlushViewportResults(res, false /* oldHouseSearch */);
}

void SearchQueryV2::ClearCaches()
{
  Query::ClearCaches();
  m_geocoder.ClearCaches();
}

void SearchQueryV2::AddPreResults1(vector<FeatureID> & results)
{
  // Group all features by MwmId and add them as PreResult1.
  sort(results.begin(), results.end());

  auto ib = results.begin();
  while (ib != results.end())
  {
    auto ie = ib;
    while (ie != results.end() && ie->m_mwmId == ib->m_mwmId)
      ++ie;

    /// @todo Add RankTableCache here?
    MwmSet::MwmHandle handle = m_index.GetMwmHandleById(ib->m_mwmId);
    if (handle.IsAlive())
    {
      auto rankTable = RankTable::Load(handle.GetValue<MwmValue>()->m_cont);
      if (!rankTable.get())
        rankTable.reset(new DummyRankTable());

      for (auto ii = ib; ii != ie; ++ii)
        AddPreResult1(ii->m_mwmId, ii->m_index, rankTable->Get(ii->m_index), 0.0 /* priority */);
    }
    ib = ie;
  }
}
}  // namespace v2
}  // namespace search
