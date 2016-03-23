#include "search/v2/locality_scorer.hpp"

#include "std/algorithm.hpp"

namespace search
{
namespace v2
{
// static
size_t const LocalityScorer::kDefaultReadLimit = 100;

namespace
{
bool IsAlmostFullMatch(NameScore score)
{
  return score == NAME_SCORE_FULL_MATCH_PREFIX || score == NAME_SCORE_FULL_MATCH;
}
}  // namespace

// LocalityScorer::ExLocality ----------------------------------------------------------------------
LocalityScorer::ExLocality::ExLocality() : m_numTokens(0), m_rank(0), m_nameScore(NAME_SCORE_ZERO)
{
}

LocalityScorer::ExLocality::ExLocality(Geocoder::Locality const & locality)
  : m_locality(locality)
  , m_numTokens(locality.m_endToken - locality.m_startToken)
  , m_rank(0)
  , m_nameScore(NAME_SCORE_ZERO)
{
}

// LocalityScorer ----------------------------------------------------------------------------------
LocalityScorer::LocalityScorer(SearchQueryParams const & params, Delegate const & delegate)
  : m_params(params), m_delegate(delegate)
{
}

void LocalityScorer::GetTopLocalities(size_t limit, vector<Geocoder::Locality> & localities) const
{
  vector<ExLocality> ls;
  ls.reserve(localities.size());
  for (auto const & locality : localities)
    ls.emplace_back(locality);

  RemoveDuplicates(ls);
  LeaveTopByRank(std::max(limit, kDefaultReadLimit), ls);
  SortByName(ls);
  if (ls.size() > limit)
    ls.resize(limit);

  localities.clear();
  localities.reserve(ls.size());
  for (auto const & l : ls)
    localities.push_back(l.m_locality);
}

void LocalityScorer::RemoveDuplicates(vector<ExLocality> & ls) const
{
  sort(ls.begin(), ls.end(), [](ExLocality const & lhs, ExLocality const & rhs)
       {
         if (lhs.GetId() != rhs.GetId())
           return lhs.GetId() < rhs.GetId();
         return lhs.m_numTokens > rhs.m_numTokens;
       });
  ls.erase(unique(ls.begin(), ls.end(),
                  [](ExLocality const & lhs, ExLocality const & rhs)
                  {
                    return lhs.GetId() == rhs.GetId();
                  }),
           ls.end());
}

void LocalityScorer::LeaveTopByRank(size_t limit, vector<ExLocality> & ls) const
{
  if (ls.size() <= limit)
    return;

  for (auto & l : ls)
    l.m_rank = m_delegate.GetRank(l.GetId());

  sort(ls.begin(), ls.end(), [](ExLocality const & lhs, ExLocality const & rhs)
       {
         if (lhs.m_rank != rhs.m_rank)
           return lhs.m_rank > rhs.m_rank;
         return lhs.m_numTokens > rhs.m_numTokens;
       });
  ls.resize(limit);
}

void LocalityScorer::SortByName(vector<ExLocality> & ls) const
{
  vector<string> names;
  for (auto & l : ls)
  {
    names.clear();
    m_delegate.GetNames(l.GetId(), names);

    auto score = NAME_SCORE_ZERO;
    for (auto const & name : names)
    {
      score = max(score, GetNameScore(name, v2::TokensSlice(m_params, l.m_locality.m_startToken,
                                                            l.m_locality.m_endToken)));
    }
    l.m_nameScore = score;
  }

  sort(ls.begin(), ls.end(), [](ExLocality const & lhs, ExLocality const & rhs)
  {
    if (IsAlmostFullMatch(lhs.m_nameScore) && IsAlmostFullMatch(rhs.m_nameScore))
    {
      // When both localities match well, e.g. full or full prefix
      // match, the one with larger number of tokens is selected. In
      // case of tie, the one with better score is selected.
      if (lhs.m_numTokens != rhs.m_numTokens)
        return lhs.m_numTokens > rhs.m_numTokens;
      if (lhs.m_nameScore != rhs.m_nameScore)
        return lhs.m_nameScore > rhs.m_nameScore;
    }
    else
    {
      // When name scores differ, the one with better name score is
      // selected.  In case of tie, the one with larger number of
      // matched tokens is selected.
      if (lhs.m_nameScore != rhs.m_nameScore)
        return lhs.m_nameScore > rhs.m_nameScore;
      if (lhs.m_numTokens != rhs.m_numTokens)
        return lhs.m_numTokens > rhs.m_numTokens;
    }

    // Okay, in case of tie we select the one with better rank.  This
    // is a quite arbitrary decision and definitely may be improved.
    return lhs.m_rank > rhs.m_rank;
  });
}
}  // namespace v2
}  // namespace search
