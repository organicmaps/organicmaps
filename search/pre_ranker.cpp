#include "search/pre_ranker.hpp"

#include "search/dummy_rank_table.hpp"
#include "search/pre_ranking_info.hpp"

#include "indexer/mwm_set.hpp"
#include "indexer/rank_table.hpp"

#include "base/stl_helpers.hpp"

#include "std/iterator.hpp"
#include "std/random.hpp"
#include "std/set.hpp"

namespace search
{
namespace
{
size_t const kBatchSize = 100;

struct LessFeatureID
{
  using TValue = PreResult1;

  inline bool operator()(TValue const & lhs, TValue const & rhs) const
  {
    return lhs.GetId() < rhs.GetId();
  }
};

// Orders PreResult1 by following criterion:
// 1. Feature Id (increasing), if same...
// 2. Number of matched tokens from the query (decreasing), if same...
// 3. Index of the first matched token from the query (increasing).
struct ComparePreResult1
{
  bool operator()(PreResult1 const & lhs, PreResult1 const & rhs) const
  {
    if (lhs.GetId() != rhs.GetId())
      return lhs.GetId() < rhs.GetId();

    auto const & linfo = lhs.GetInfo();
    auto const & rinfo = rhs.GetInfo();
    if (linfo.GetNumTokens() != rinfo.GetNumTokens())
      return linfo.GetNumTokens() > rinfo.GetNumTokens();
    return linfo.m_startToken < rinfo.m_startToken;
  }
};
}  // namespace

PreRanker::PreRanker(Index const & index, Ranker & ranker, size_t limit)
  : m_index(index), m_ranker(ranker), m_limit(limit), m_pivotFeatures(index)
{
}

void PreRanker::Init(Params const & params)
{
  m_numSentResults = 0;
  m_results.clear();
  m_params = params;
}

void PreRanker::FillMissingFieldsInPreResults()
{
  MwmSet::MwmId mwmId;
  MwmSet::MwmHandle mwmHandle;
  unique_ptr<RankTable> rankTable = make_unique<DummyRankTable>();

  ForEach([&](PreResult1 & r) {
    FeatureID const & id = r.GetId();
    PreRankingInfo & info = r.GetInfo();
    if (id.m_mwmId != mwmId)
    {
      mwmId = id.m_mwmId;
      mwmHandle = m_index.GetMwmHandleById(mwmId);
      rankTable.reset();
      if (mwmHandle.IsAlive())
      {
        rankTable = RankTable::Load(mwmHandle.GetValue<MwmValue>()->m_cont);
      }
      if (!rankTable)
        rankTable = make_unique<DummyRankTable>();
    }

    info.m_rank = rankTable->Get(id.m_index);
  });

  if (Size() <= kBatchSize)
    return;

  m_pivotFeatures.SetPosition(m_params.m_accuratePivotCenter, m_params.m_scale);
  ForEach([&](PreResult1 & r) {
    FeatureID const & id = r.GetId();
    PreRankingInfo & info = r.GetInfo();

    info.m_distanceToPivot = m_pivotFeatures.GetDistanceToFeatureMeters(id);
  });
}

void PreRanker::Filter(bool viewportSearch)
{
  using TSet = set<PreResult1, LessFeatureID>;
  TSet filtered;

  sort(m_results.begin(), m_results.end(), ComparePreResult1());
  m_results.erase(unique(m_results.begin(), m_results.end(), my::EqualsBy(&PreResult1::GetId)),
                  m_results.end());

  sort(m_results.begin(), m_results.end(), &PreResult1::LessDistance);

  if (m_results.size() > kBatchSize)
  {
    // Priority is some kind of distance from the viewport or
    // position, therefore if we have a bunch of results with the same
    // priority, we have no idea here which results are relevant.  To
    // prevent bias from previous search routines (like sorting by
    // feature id) this code randomly selects tail of the
    // sorted-by-priority list of pre-results.

    double const last = m_results[kBatchSize - 1].GetDistance();

    auto b = m_results.begin() + kBatchSize - 1;
    for (; b != m_results.begin() && b->GetDistance() == last; --b)
      ;
    if (b->GetDistance() != last)
      ++b;

    auto e = m_results.begin() + kBatchSize;
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
    minstd_rand engine;
    shuffle(b, e, engine);
  }
  filtered.insert(m_results.begin(), m_results.begin() + min(m_results.size(), kBatchSize));

  if (!viewportSearch)
  {
    size_t n = min(m_results.size(), kBatchSize);
    nth_element(m_results.begin(), m_results.begin() + n, m_results.end(), &PreResult1::LessRank);
    filtered.insert(m_results.begin(), m_results.begin() + n);
  }

  m_results.reserve(filtered.size());
  m_results.clear();
  copy(filtered.begin(), filtered.end(), back_inserter(m_results));
}

void PreRanker::UpdateResults(bool lastUpdate)
{
  FillMissingFieldsInPreResults();
  Filter(m_viewportSearch);
  m_numSentResults += m_results.size();
  m_ranker.SetPreResults1(move(m_results));
  m_results.clear();
  m_ranker.UpdateResults(lastUpdate);
}

void PreRanker::ClearCaches() { m_pivotFeatures.Clear(); }
}  // namespace search
