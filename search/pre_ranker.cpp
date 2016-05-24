#include "search/pre_ranker.hpp"

#include "search/v2/pre_ranking_info.hpp"

#include "std/iterator.hpp"
#include "std/random.hpp"
#include "std/set.hpp"

namespace search
{
namespace
{
struct LessFeatureID
{
  using TValue = impl::PreResult1;

  inline bool operator()(TValue const & lhs, TValue const & rhs) const
  {
    return lhs.GetId() < rhs.GetId();
  }
};

struct EqualFeatureID
{
  using TValue = impl::PreResult1;

  inline bool operator()(TValue const & lhs, TValue const & rhs) const
  {
    return lhs.GetId() == rhs.GetId();
  }
};

// Orders PreResult1 by following criterion:
// 1. Feature Id (increasing), if same...
// 2. Number of matched tokens from the query (decreasing), if same...
// 3. Index of the first matched token from the query (increasing).
struct ComparePreResult1
{
  bool operator()(impl::PreResult1 const & lhs, impl::PreResult1 const & rhs) const
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

PreRanker::PreRanker(size_t limit) : m_limit(limit) {}

void PreRanker::Add(impl::PreResult1 const & result) { m_results.push_back(result); }

void PreRanker::Filter(bool viewportSearch)
{
  using TSet = set<impl::PreResult1, LessFeatureID>;
  TSet theSet;

  sort(m_results.begin(), m_results.end(), ComparePreResult1());
  m_results.erase(unique(m_results.begin(), m_results.end(), EqualFeatureID()), m_results.end());

  sort(m_results.begin(), m_results.end(), &impl::PreResult1::LessDistance);

  if (m_limit != 0 && m_results.size() > m_limit)
  {
    // Priority is some kind of distance from the viewport or
    // position, therefore if we have a bunch of results with the same
    // priority, we have no idea here which results are relevant.  To
    // prevent bias from previous search routines (like sorting by
    // feature id) this code randomly selects tail of the
    // sorted-by-priority list of pre-results.

    double const last = m_results[m_limit - 1].GetDistance();

    auto b = m_results.begin() + m_limit - 1;
    for (; b != m_results.begin() && b->GetDistance() == last; --b)
      ;
    if (b->GetDistance() != last)
      ++b;

    auto e = m_results.begin() + m_limit;
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
  theSet.insert(m_results.begin(), m_results.begin() + min(m_results.size(), m_limit));

  if (!viewportSearch)
  {
    size_t n = min(m_results.size(), m_limit);
    nth_element(m_results.begin(), m_results.begin() + n, m_results.end(),
                &impl::PreResult1::LessRank);
    theSet.insert(m_results.begin(), m_results.begin() + n);
  }

  m_results.reserve(theSet.size());
  m_results.clear();
  copy(theSet.begin(), theSet.end(), back_inserter(m_results));
}
}  // namespace search
