#include "search/pre_ranker.hpp"

#include "search/dummy_rank_table.hpp"
#include "search/lazy_centers_table.hpp"
#include "search/pre_ranking_info.hpp"

#include "indexer/data_source.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/rank_table.hpp"
#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"
#include "geometry/nearby_points_sweeper.hpp"

#include "base/random.hpp"
#include "base/stl_helpers.hpp"

#include <iterator>
#include <set>

using namespace std;

namespace search
{
namespace
{
void SweepNearbyResults(double eps, set<FeatureID> const & prevEmit, vector<PreRankerResult> & results)
{
  m2::NearbyPointsSweeper sweeper(eps);
  for (size_t i = 0; i < results.size(); ++i)
  {
    auto const & p = results[i].GetInfo().m_center;
    uint8_t const rank = results[i].GetInfo().m_rank;
    uint8_t const popularity = results[i].GetInfo().m_popularity;
    uint8_t const prevCount = prevEmit.count(results[i].GetId()) ? 1 : 0;
    uint8_t const priority = max({rank, prevCount, popularity});
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
  m_results.clear();
  m_params = params;
  m_currEmit.clear();
}

void PreRanker::Finish(bool cancelled)
{
  if (!cancelled)
    UpdateResults(true /* lastUpdate */);

  m_ranker.Finish(cancelled);
}

void PreRanker::FillMissingFieldsInPreResults()
{
  MwmSet::MwmId mwmId;
  MwmSet::MwmHandle mwmHandle;
  unique_ptr<RankTable> ranks = make_unique<DummyRankTable>();
  unique_ptr<RankTable> popularityRanks = make_unique<DummyRankTable>();
  unique_ptr<LazyCentersTable> centers;

  m_pivotFeatures.SetPosition(m_params.m_accuratePivotCenter, m_params.m_scale);

  ForEach([&](PreRankerResult & r) {
    FeatureID const & id = r.GetId();
    PreRankingInfo & info = r.GetInfo();
    if (id.m_mwmId != mwmId)
    {
      mwmId = id.m_mwmId;
      mwmHandle = m_dataSource.GetMwmHandleById(mwmId);
      ranks.reset();
      centers.reset();
      if (mwmHandle.IsAlive())
      {
        ranks = RankTable::Load(mwmHandle.GetValue<MwmValue>()->m_cont, SEARCH_RANKS_FILE_TAG);
        popularityRanks = RankTable::Load(mwmHandle.GetValue<MwmValue>()->m_cont,
                                          POPULARITY_RANKS_FILE_TAG);
        centers = make_unique<LazyCentersTable>(*mwmHandle.GetValue<MwmValue>());
      }
      if (!ranks)
        ranks = make_unique<DummyRankTable>();
      if (!popularityRanks)
        popularityRanks = make_unique<DummyRankTable>();
    }

    info.m_rank = ranks->Get(id.m_index);
    info.m_popularity = popularityRanks->Get(id.m_index);

    m2::PointD center;
    if (centers && centers->Get(id.m_index, center))
    {
      info.m_distanceToPivot =
          MercatorBounds::DistanceOnEarth(m_params.m_accuratePivotCenter, center);
      info.m_center = center;
      info.m_centerLoaded = true;
    }
    else
    {
      info.m_distanceToPivot = m_pivotFeatures.GetDistanceToFeatureMeters(id);
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

    auto const & linfo = lhs.GetInfo();
    auto const & rinfo = rhs.GetInfo();
    if (linfo.GetNumTokens() != rinfo.GetNumTokens())
      return linfo.GetNumTokens() > rinfo.GetNumTokens();
    return linfo.InnermostTokenRange().Begin() < rinfo.InnermostTokenRange().Begin();
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
    }
    else
    {
      double const kPedestrianRadiusMeters = 2500.0;
      PreRankerResult::CategoriesComparator comparator;
      comparator.m_positionIsInsideViewport =
          m_params.m_position && m_params.m_viewport.IsPointInside(*m_params.m_position);
      comparator.m_detailedScale = MercatorBounds::DistanceOnEarth(
                                       m_params.m_viewport.LeftTop(),
                                       m_params.m_viewport.RightBottom()) < 2 * kPedestrianRadiusMeters;
      comparator.m_viewport = m_params.m_viewport;
      nth_element(m_results.begin(), m_results.begin() + numResults, m_results.end(), comparator);
    }

    filtered.insert(m_results.begin(), m_results.begin() + numResults);
  }

  m_results.assign(filtered.begin(), filtered.end());
}

void PreRanker::UpdateResults(bool lastUpdate)
{
  FillMissingFieldsInPreResults();
  Filter(m_params.m_viewportSearch);
  m_numSentResults += m_results.size();
  m_ranker.SetPreRankerResults(move(m_results));
  m_results.clear();
  m_ranker.UpdateResults(lastUpdate);

  if (lastUpdate)
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

  base::EraseIf(m_results, [&viewport](PreRankerResult const & result) {
    auto const & info = result.GetInfo();
    return !viewport.IsPointInside(info.m_center);
  });

  SweepNearbyResults(m_params.m_minDistanceOnMapBetweenResults, m_prevEmit, m_results);

  size_t const n = m_results.size();

  if (n <= BatchSize())
    return;

  size_t const kNumXSlots = 5;
  size_t const kNumYSlots = 5;
  size_t const kNumBuckets = kNumXSlots * kNumYSlots;
  vector<size_t> buckets[kNumBuckets];

  double const sizeX = viewport.SizeX();
  double const sizeY = viewport.SizeY();

  for (size_t i = 0; i < m_results.size(); ++i)
  {
    auto const & p = m_results[i].GetInfo().m_center;
    int dx = static_cast<int>((p.x - viewport.minX()) / sizeX * kNumXSlots);
    dx = base::clamp(dx, 0, static_cast<int>(kNumXSlots) - 1);

    int dy = static_cast<int>((p.y - viewport.minY()) / sizeY * kNumYSlots);
    dy = base::clamp(dy, 0, static_cast<int>(kNumYSlots) - 1);

    buckets[dx * kNumYSlots + dy].push_back(i);
  }

  vector<PreRankerResult> results;
  double const density = static_cast<double>(BatchSize()) / static_cast<double>(n);
  for (auto & bucket : buckets)
  {
    size_t const m = std::min(static_cast<size_t>(ceil(density * bucket.size())), bucket.size());

    size_t const old =
        partition(bucket.begin(), bucket.end(),
                  [this](size_t i) { return m_prevEmit.count(m_results[i].GetId()) != 0; }) -
        bucket.begin();

    if (m <= old)
    {
      for (size_t i : base::RandomSample(old, m, m_rng))
        results.push_back(m_results[bucket[i]]);
    }
    else
    {
      for (size_t i = 0; i < old; ++i)
        results.push_back(m_results[bucket[i]]);

      for (size_t i : base::RandomSample(bucket.size() - old, m - old, m_rng))
        results.push_back(m_results[bucket[old + i]]);
    }
  }

  if (results.size() <= BatchSize())
  {
    m_results.swap(results);
  }
  else
  {
    m_results.clear();
    for (size_t i : base::RandomSample(results.size(), BatchSize(), m_rng))
      m_results.push_back(results[i]);
  }
}
}  // namespace search
