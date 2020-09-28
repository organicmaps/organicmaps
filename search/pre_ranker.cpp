#include "search/pre_ranker.hpp"

#include "search/dummy_rank_table.hpp"
#include "search/lazy_centers_table.hpp"
#include "search/pre_ranking_info.hpp"
#include "search/tracer.hpp"

#include "ugc/types.hpp"

#include "editor/osm_editor.hpp"

#include "indexer/data_source.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/rank_table.hpp"
#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"
#include "geometry/nearby_points_sweeper.hpp"

#include "base/random.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <iterator>
#include <set>

using namespace std;

namespace search
{
namespace
{
void SweepNearbyResults(double xEps, double yEps, set<FeatureID> const & prevEmit,
                        vector<PreRankerResult> & results)
{
  m2::NearbyPointsSweeper sweeper(xEps, yEps);
  for (size_t i = 0; i < results.size(); ++i)
  {
    auto const & p = results[i].GetInfo().m_center;
    uint8_t const rank = results[i].GetInfo().m_rank;
    uint8_t const popularity = results[i].GetInfo().m_popularity;
    uint8_t const exactMatch = results[i].GetInfo().m_exactMatch ? 1 : 0;
    // We prefer result which passed the filter even if it has lower rank / popularity / exactMatch.
    uint8_t const filterPassed = results[i].GetInfo().m_refusedByFilter ? 0 : 2;
    // We prefer result from prevEmit over result with better filterPassed because otherwise we have
    // lots of blinking results.
    uint8_t const prevCount = prevEmit.count(results[i].GetId()) == 0 ? 0 : 3;
    uint8_t const priority = max({rank, prevCount, popularity, exactMatch}) + filterPassed;
    sweeper.Add(p.x, p.y, i, priority);
  }

  vector<PreRankerResult> filtered;
  sweeper.Sweep([&filtered, &results](size_t i)
                {
                  filtered.push_back(results[i]);
                });

  results.swap(filtered);
}
}  // namespace

PreRanker::PreRanker(DataSource const & dataSource, Ranker & ranker)
  : m_dataSource(dataSource), m_ranker(ranker), m_pivotFeatures(dataSource)
{
}

void PreRanker::Init(Params const & params)
{
  m_numSentResults = 0;
  m_haveFullyMatchedResult = false;
  m_results.clear();
  m_relaxedResults.clear();
  m_params = params;
  m_currEmit.clear();
}

void PreRanker::Finish(bool cancelled)
{
  m_ranker.Finish(cancelled);
}

void PreRanker::FillMissingFieldsInPreResults()
{
  MwmSet::MwmId mwmId;
  MwmSet::MwmHandle mwmHandle;
  unique_ptr<RankTable> ranks = make_unique<DummyRankTable>();
  unique_ptr<RankTable> popularityRanks = make_unique<DummyRankTable>();
  unique_ptr<RankTable> ratings = make_unique<DummyRankTable>();
  unique_ptr<LazyCentersTable> centers;
  bool pivotFeaturesInitialized = false;

  ForEach([&](PreRankerResult & r) {
    FeatureID const & id = r.GetId();
    if (id.m_mwmId != mwmId)
    {
      mwmId = id.m_mwmId;
      mwmHandle = m_dataSource.GetMwmHandleById(mwmId);
      ranks.reset();
      centers.reset();
      if (mwmHandle.IsAlive())
      {
        ranks = RankTable::Load(mwmHandle.GetValue()->m_cont, SEARCH_RANKS_FILE_TAG);
        popularityRanks = RankTable::Load(mwmHandle.GetValue()->m_cont,
                                          POPULARITY_RANKS_FILE_TAG);
        ratings = RankTable::Load(mwmHandle.GetValue()->m_cont, RATINGS_FILE_TAG);
        centers = make_unique<LazyCentersTable>(*mwmHandle.GetValue());
      }
      if (!ranks)
        ranks = make_unique<DummyRankTable>();
      if (!popularityRanks)
        popularityRanks = make_unique<DummyRankTable>();
      if (!ratings)
        ratings = make_unique<DummyRankTable>();
    }

    r.SetRank(ranks->Get(id.m_index));
    r.SetPopularity(popularityRanks->Get(id.m_index));
    r.SetRating(ugc::UGC::UnpackRating(ratings->Get(id.m_index)));

    m2::PointD center;
    if (centers && centers->Get(id.m_index, center))
    {
      r.SetDistanceToPivot(mercator::DistanceOnEarth(m_params.m_accuratePivotCenter, center));
      r.SetCenter(center);
    }
    else
    {
      auto const & editor = osm::Editor::Instance();
      if (editor.GetFeatureStatus(id.m_mwmId, id.m_index) == FeatureStatus::Created)
      {
        auto const emo = editor.GetEditedFeature(id);
        CHECK(emo, ());
        center = emo->GetMercator();
        r.SetDistanceToPivot(mercator::DistanceOnEarth(m_params.m_accuratePivotCenter, center));
        r.SetCenter(center);
      }
      else
      {
        if (!pivotFeaturesInitialized)
        {
          m_pivotFeatures.SetPosition(m_params.m_accuratePivotCenter, m_params.m_scale);
          pivotFeaturesInitialized = true;
        }
        r.SetDistanceToPivot(m_pivotFeatures.GetDistanceToFeatureMeters(id));
      }
    }
  });
}

void PreRanker::Filter(bool viewportSearch)
{
  struct LessFeatureID
  {
    inline bool operator()(PreRankerResult const & lhs, PreRankerResult const & rhs) const
    {
      return lhs.GetId() < rhs.GetId();
    }
  };

  auto comparePreRankerResults = [](PreRankerResult const & lhs,
                                    PreRankerResult const & rhs) -> bool {
    if (lhs.GetId() != rhs.GetId())
      return lhs.GetId() < rhs.GetId();

    if (lhs.GetInnermostTokensNumber() != rhs.GetInnermostTokensNumber())
      return lhs.GetInnermostTokensNumber() > rhs.GetInnermostTokensNumber();

    if (lhs.GetMatchedTokensNumber() != rhs.GetMatchedTokensNumber())
      return lhs.GetMatchedTokensNumber() > rhs.GetMatchedTokensNumber();

    return lhs.GetInfo().InnermostTokenRange().Begin() < rhs.GetInfo().InnermostTokenRange().Begin();
  };

  sort(m_results.begin(), m_results.end(), comparePreRankerResults);
  m_results.erase(unique(m_results.begin(), m_results.end(), base::EqualsBy(&PreRankerResult::GetId)),
                  m_results.end());

  bool const centersLoaded =
      all_of(m_results.begin(), m_results.end(),
             [](PreRankerResult const & result) { return result.GetInfo().m_centerLoaded; });
  if (viewportSearch && centersLoaded)
  {
    FilterForViewportSearch();
    ASSERT_LESS_OR_EQUAL(m_results.size(), BatchSize(), ());
    for (auto const & result : m_results)
      m_currEmit.insert(result.GetId());
  }
  else if (m_results.size() > BatchSize())
  {
    sort(m_results.begin(), m_results.end(), &PreRankerResult::LessDistance);

    // Priority is some kind of distance from the viewport or
    // position, therefore if we have a bunch of results with the same
    // priority, we have no idea here which results are relevant.  To
    // prevent bias from previous search routines (like sorting by
    // feature id) this code randomly selects tail of the
    // sorted-by-priority list of pre-results.

    double const last = m_results[BatchSize()].GetDistance();

    auto b = m_results.begin() + BatchSize();
    for (; b != m_results.begin() && b->GetDistance() == last; --b)
      ;
    if (b->GetDistance() != last)
      ++b;

    auto e = m_results.begin() + BatchSize();
    for (; e != m_results.end() && e->GetDistance() == last; ++e)
      ;

    // The main reason of shuffling here is to select a random subset
    // from the low-priority results. We're using a linear
    // congruential method with default seed because it is fast,
    // simple and doesn't need an external entropy source.
    //
    // TODO (@y, @m, @vng): consider to take some kind of hash from
    // features and then select a subset with smallest values of this
    // hash.  In this case this subset of results will be persistent
    // to small changes in the original set.
    shuffle(b, e, m_rng);
  }

  set<PreRankerResult, LessFeatureID> filtered;

  auto const numResults = min(m_results.size(), BatchSize());
  filtered.insert(m_results.begin(), m_results.begin() + numResults);

  if (!viewportSearch)
  {
    if (!m_params.m_categorialRequest)
    {
      nth_element(m_results.begin(), m_results.begin() + numResults, m_results.end(),
                  &PreRankerResult::LessRankAndPopularity);
      filtered.insert(m_results.begin(), m_results.begin() + numResults);
      nth_element(m_results.begin(), m_results.begin() + numResults, m_results.end(),
                  &PreRankerResult::LessByExactMatch);
      filtered.insert(m_results.begin(), m_results.begin() + numResults);
    }
    else
    {
      double const kPedestrianRadiusMeters = 2500.0;
      PreRankerResult::CategoriesComparator comparator;
      comparator.m_positionIsInsideViewport =
          m_params.m_position && m_params.m_viewport.IsPointInside(*m_params.m_position);
      comparator.m_detailedScale = mercator::DistanceOnEarth(m_params.m_viewport.LeftTop(),
                                                             m_params.m_viewport.RightBottom()) <
                                   2 * kPedestrianRadiusMeters;
      comparator.m_viewport = m_params.m_viewport;
      nth_element(m_results.begin(), m_results.begin() + numResults, m_results.end(), comparator);
      filtered.insert(m_results.begin(), m_results.begin() + numResults);
    }
  }

  m_results.assign(filtered.begin(), filtered.end());
}

void PreRanker::UpdateResults(bool lastUpdate)
{
  FilterRelaxedResults(lastUpdate);
  FillMissingFieldsInPreResults();
  Filter(m_params.m_viewportSearch);
  m_numSentResults += m_results.size();
  m_ranker.AddPreRankerResults(move(m_results));
  m_results.clear();
  m_ranker.UpdateResults(lastUpdate);

  if (lastUpdate && !m_currEmit.empty())
    m_currEmit.swap(m_prevEmit);
}

void PreRanker::ClearCaches()
{
  m_pivotFeatures.Clear();
  m_prevEmit.clear();
  m_currEmit.clear();
}

void PreRanker::FilterForViewportSearch()
{
  auto const & viewport = m_params.m_viewport;

  base::EraseIf(m_results, [&](PreRankerResult const & result) {
    auto const & info = result.GetInfo();
    if (!viewport.IsPointInside(info.m_center))
      return true;

    return result.GetMatchedTokensNumber() + 1 < m_params.m_numQueryTokens;
  });

  SweepNearbyResults(m_params.m_minDistanceOnMapBetweenResultsX,
                     m_params.m_minDistanceOnMapBetweenResultsY, m_prevEmit, m_results);

  CHECK_LESS_OR_EQUAL(m_results.size(), BatchSize(), ());
}

void PreRanker::FilterRelaxedResults(bool lastUpdate)
{
  if (lastUpdate)
  {
    m_results.insert(m_results.end(), m_relaxedResults.begin(), m_relaxedResults.end());
    m_relaxedResults.clear();
  }
  else
  {
    auto const isNotRelaxed = [](PreRankerResult const & res) {
      auto const & prov = res.GetProvenance();
      return find(prov.begin(), prov.end(), ResultTracer::Branch::Relaxed) == prov.end();
    };
    auto const it = partition(m_results.begin(), m_results.end(), isNotRelaxed);
    m_relaxedResults.insert(m_relaxedResults.end(), it, m_results.end());
    m_results.erase(it, m_results.end());
  }
}
}  // namespace search
