#include "search/locality_scorer.hpp"

#include "search/cbv.hpp"
#include "search/geocoder_context.hpp"
#include "search/token_slice.hpp"

#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace search
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

LocalityScorer::ExLocality::ExLocality(Locality const & locality)
  : m_locality(locality)
  , m_numTokens(locality.m_tokenRange.Size())
  , m_rank(0)
  , m_nameScore(NAME_SCORE_ZERO)
{
}

// LocalityScorer ----------------------------------------------------------------------------------
LocalityScorer::LocalityScorer(QueryParams const & params, Delegate const & delegate)
  : m_params(params), m_delegate(delegate)
{
}

void LocalityScorer::GetTopLocalities(MwmSet::MwmId const & countryId, BaseContext const & ctx,
                                      CBV const & filter, size_t limit,
                                      std::vector<Locality> & localities)
{
  CHECK_EQUAL(ctx.m_numTokens, m_params.GetNumTokens(), ());

  localities.clear();

  for (size_t startToken = 0; startToken < ctx.m_numTokens; ++startToken)
  {
    CBV intersection = filter.Intersect(ctx.m_features[startToken]);
    if (intersection.IsEmpty())
      continue;

    CBV unfilteredIntersection = ctx.m_features[startToken];

    for (size_t endToken = startToken + 1; endToken <= ctx.m_numTokens; ++endToken)
    {
      TokenRange const tokenRange(startToken, endToken);
      // Skip locality candidates that match only numbers.
      if (!m_params.IsNumberTokens(tokenRange))
      {
        intersection.ForEach([&](uint32_t featureId) {
          double const prob = static_cast<double>(intersection.PopCount()) /
                              static_cast<double>(unfilteredIntersection.PopCount());
          localities.emplace_back(countryId, featureId, tokenRange, prob);
        });
      }

      if (endToken < ctx.m_numTokens)
      {
        intersection = intersection.Intersect(ctx.m_features[endToken]);
        if (intersection.IsEmpty())
          break;

        unfilteredIntersection = unfilteredIntersection.Intersect(ctx.m_features[endToken]);
      }
    }
  }

  LeaveTopLocalities(limit, localities);
}

void LocalityScorer::LeaveTopLocalities(size_t limit, std::vector<Locality> & localities) const
{
  std::vector<ExLocality> ls;
  ls.reserve(localities.size());
  for (auto const & locality : localities)
    ls.emplace_back(locality);

  RemoveDuplicates(ls);
  LeaveTopByRankAndProb(std::max(limit, kDefaultReadLimit), ls);
  SortByNameAndProb(ls);
  if (ls.size() > limit)
    ls.resize(limit);

  localities.clear();
  localities.reserve(ls.size());
  for (auto const & l : ls)
    localities.push_back(l.m_locality);
}

void LocalityScorer::RemoveDuplicates(std::vector<ExLocality> & ls) const
{
  std::sort(ls.begin(), ls.end(), [](ExLocality const & lhs, ExLocality const & rhs) {
    if (lhs.GetId() != rhs.GetId())
      return lhs.GetId() < rhs.GetId();
    return lhs.m_numTokens > rhs.m_numTokens;
  });
  ls.erase(std::unique(ls.begin(), ls.end(),
                       [](ExLocality const & lhs, ExLocality const & rhs) {
                         return lhs.GetId() == rhs.GetId();
                       }),
           ls.end());
}

void LocalityScorer::LeaveTopByRankAndProb(size_t limit, std::vector<ExLocality> & ls) const
{
  if (ls.size() <= limit)
    return;

  for (auto & l : ls)
    l.m_rank = m_delegate.GetRank(l.GetId());

  std::sort(ls.begin(), ls.end(), [](ExLocality const & lhs, ExLocality const & rhs) {
    if (lhs.m_locality.m_prob != rhs.m_locality.m_prob)
      return lhs.m_locality.m_prob > rhs.m_locality.m_prob;
    if (lhs.m_rank != rhs.m_rank)
      return lhs.m_rank > rhs.m_rank;
    return lhs.m_numTokens > rhs.m_numTokens;
  });
  ls.resize(limit);
}

void LocalityScorer::SortByNameAndProb(std::vector<ExLocality> & ls) const
{
  std::vector<std::string> names;
  for (auto & l : ls)
  {
    names.clear();
    m_delegate.GetNames(l.GetId(), names);

    auto score = NAME_SCORE_ZERO;
    for (auto const & name : names)
      score = max(score, GetNameScore(name, TokenSlice(m_params, l.m_locality.m_tokenRange)));
    l.m_nameScore = score;

    std::sort(ls.begin(), ls.end(), [](ExLocality const & lhs, ExLocality const & rhs) {
      // Probabilities form a stronger signal than name scores do.
      if (lhs.m_locality.m_prob != rhs.m_locality.m_prob)
        return lhs.m_locality.m_prob > rhs.m_locality.m_prob;
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
}

string DebugPrint(LocalityScorer::ExLocality const & locality)
{
  ostringstream os;
  os << "LocalityScorer::ExLocality [ ";
  os << "m_locality=" << DebugPrint(locality.m_locality) << ", ";
  os << "m_numTokens=" << locality.m_numTokens << ", ";
  os << "m_rank=" << static_cast<uint32_t>(locality.m_rank) << ", ";
  os << "m_nameScore=" << DebugPrint(locality.m_nameScore);
  os << " ]";
  return os.str();
}
}  // namespace search
