#include "search/locality_scorer.hpp"

#include "search/cbv.hpp"
#include "search/geocoder_context.hpp"
#include "search/idf_map.hpp"
#include "search/ranking_utils.hpp"
#include "search/retrieval.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/dfa_helpers.hpp"
#include "base/levenshtein_dfa.hpp"

#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace search
{
using namespace std;
using namespace strings;

using PrefixDFA = PrefixDFAModifier<LevenshteinDFA>;

namespace
{
class IdfMapDelegate : public IdfMap::Delegate
{
public:
  IdfMapDelegate(vector<pair<LevenshteinDFA, uint64_t>> const & tokensToDf,
                 vector<pair<PrefixDFA, uint64_t>> const & prefixToDf)
    : m_tokensToDf(tokensToDf)
    , m_prefixToDf(prefixToDf)
  {}

  ~IdfMapDelegate() override = default;

  uint64_t GetNumDocs(UniString const & token, bool isPrefix) const override
  {
    if (isPrefix)
    {
      for (auto const & dfa : m_prefixToDf)
      {
        auto it = dfa.first.Begin();
        DFAMove(it, token);
        if (it.Accepts())
          return dfa.second;
      }
      return 0;
    }

    for (auto const & dfa : m_tokensToDf)
    {
      auto it = dfa.first.Begin();
      DFAMove(it, token);
      if (it.Accepts())
        return dfa.second;
    }
    return 0;
  }

private:
  vector<pair<LevenshteinDFA, uint64_t>> const & m_tokensToDf;
  vector<pair<PrefixDFA, uint64_t>> const & m_prefixToDf;
};
}  // namespace

// Used as a default (minimun) number of candidates for futher processing (read FeatureType).
size_t constexpr kDefaultReadLimit = 100;

// LocalityScorer::ExLocality ----------------------------------------------------------------------
LocalityScorer::ExLocality::ExLocality(Locality && locality, double queryNorm, uint8_t rank)
  : m_locality(std::move(locality))
  , m_queryNorm(queryNorm)
  , m_rank(rank)
{}

// LocalityScorer ----------------------------------------------------------------------------------
LocalityScorer::LocalityScorer(QueryParams const & params, m2::PointD const & pivot, Delegate & delegate)
  : m_params(params)
  , m_pivot(pivot)
  , m_delegate(delegate)
{}

void LocalityScorer::GetTopLocalities(MwmSet::MwmId const & countryId, BaseContext const & ctx, CBV const & filter,
                                      size_t limit, vector<Locality> & localities)
{
  double constexpr kUnknownIdf = 1.0;
  size_t const numTokens = ctx.NumTokens();
  ASSERT_EQUAL(numTokens, m_params.GetNumTokens(), ());

  localities.clear();

  vector<Retrieval::ExtendedFeatures> intersections(numTokens);
  vector<pair<LevenshteinDFA, uint64_t>> tokensToDf;
  vector<pair<PrefixDFA, uint64_t>> prefixToDf;
  bool const havePrefix = numTokens > 0 && m_params.LastTokenIsPrefix();
  size_t const nonPrefixTokens = havePrefix ? numTokens - 1 : numTokens;
  for (size_t i = 0; i < nonPrefixTokens; ++i)
  {
    intersections[i] = ctx.m_features[i].Intersect(filter);
    auto const df = intersections[i].m_features.PopCount();
    if (df != 0)
    {
      auto const & token = m_params.GetToken(i);
      tokensToDf.emplace_back(BuildLevenshteinDFA(token.GetOriginal()), df);
      token.ForEachSynonym([&tokensToDf, &df](UniString const & s)
      { tokensToDf.emplace_back(strings::LevenshteinDFA(s, 0 /* maxErrors */), df); });
    }
  }

  if (havePrefix)
  {
    auto const count = numTokens - 1;
    intersections[count] = ctx.m_features[count].Intersect(filter);
    auto const prefixDf = intersections[count].m_features.PopCount();
    if (prefixDf != 0)
    {
      auto const & token = m_params.GetToken(count);
      prefixToDf.emplace_back(PrefixDFA(BuildLevenshteinDFA(token.GetOriginal())), prefixDf);
      token.ForEachSynonym([&prefixToDf, &prefixDf](UniString const & s)
      { prefixToDf.emplace_back(PrefixDFA(strings::LevenshteinDFA(s, 0 /* maxErrors */)), prefixDf); });
    }
  }

  IdfMapDelegate delegate(tokensToDf, prefixToDf);
  IdfMap idfs(delegate, kUnknownIdf);

  for (size_t startToken = 0; startToken < numTokens; ++startToken)
  {
    auto intersection = intersections[startToken];
    QueryVec::Builder builder;

    for (size_t endToken = startToken + 1; endToken <= numTokens && !intersection.m_features.IsEmpty(); ++endToken)
    {
      auto const curToken = endToken - 1;
      auto const & token = m_params.GetToken(curToken).GetOriginal();
      if (m_params.IsPrefixToken(curToken))
        builder.SetPrefix(token);
      else
        builder.AddFull(token);

      TokenRange const tokenRange(startToken, endToken);
      // Skip locality candidates that match only numbers.
      if (!m_params.IsNumberTokens(tokenRange))
      {
        intersection.ForEach([&](uint32_t featureId, bool exactMatch)
        { localities.emplace_back(FeatureID(countryId, featureId), tokenRange, QueryVec(idfs, builder), exactMatch); });
      }

      if (endToken < numTokens)
        intersection = intersection.Intersect(intersections[endToken]);
    }
  }

  LeaveTopLocalities(idfs, limit, localities);
}

void LocalityScorer::LeaveTopLocalities(IdfMap & idfs, size_t limit, vector<Locality> & localities)
{
  vector<ExLocality> els;
  els.reserve(localities.size());
  for (auto & locality : localities)
  {
    auto const queryNorm = locality.m_queryVec.Norm();
    auto const rank = m_delegate.GetRank(locality.GetFeatureIndex());
    els.emplace_back(std::move(locality), queryNorm, rank);
  }

  // We don't want to read too many names for localities, so this is
  // the best effort - select the best features by available params -
  // exactMatch, query norm and rank.
  LeaveTopByExactMatchNormAndRank(max(limit, kDefaultReadLimit) /* limitUniqueIds */, els);

  sort(els.begin(), els.end(),
       [](ExLocality const & lhs, ExLocality const & rhs) { return lhs.GetId() < rhs.GetId(); });

  size_t i = 0;
  while (i < els.size())
  {
    size_t j = i + 1;
    while (j < els.size() && els[j].GetId() == els[i].GetId())
      ++j;

    vector<DocVec> dvs;

    // *NOTE* |idfs| is filled based on query tokens, not all
    // localities tokens, because it's expensive to compute IDF map
    // for all localities tokens.  Therefore, for tokens not in the
    // query, some default IDF value will be used.
    GetDocVecs(els[i].GetId(), dvs);

    auto const center = m_delegate.GetCenter(els[i].GetId());
    auto const distance = center ? mercator::DistanceOnEarth(m_pivot, *center) : els[i].m_distanceToPivot;
    auto const belongsToMatchedRegion =
        center ? m_delegate.BelongsToMatchedRegion(*center) : els[i].m_belongsToMatchedRegion;

    for (; i < j; ++i)
    {
      els[i].m_similarity = GetSimilarity(els[i].m_locality.m_queryVec, idfs, dvs);
      els[i].m_distanceToPivot = distance;
      els[i].m_belongsToMatchedRegion = belongsToMatchedRegion;
    }
  }

  GroupBySimilarityAndOther(els);

  localities.clear();
  localities.reserve(els.size());

  unordered_set<uint32_t> seen;
  for (auto it = els.begin(); it != els.end() && localities.size() < limit; ++it)
    if (seen.insert(it->GetId()).second)
      localities.push_back(std::move(it->m_locality));
  ASSERT_EQUAL(seen.size(), localities.size(), ());
}

void LocalityScorer::LeaveTopByExactMatchNormAndRank(size_t limitUniqueIds, vector<ExLocality> & els) const
{
  sort(els.begin(), els.end(), [](ExLocality const & lhs, ExLocality const & rhs)
  {
    if (lhs.m_locality.m_exactMatch != rhs.m_locality.m_exactMatch)
      return lhs.m_locality.m_exactMatch;
    auto const ln = lhs.m_queryNorm;
    auto const rn = rhs.m_queryNorm;
    if (ln != rn)
      return ln > rn;
    return lhs.m_rank > rhs.m_rank;
  });

  // This logic with additional filtering set makes sense when _equal_ localities by GetId()
  // have _different_ primary compare params (m_exactMatch, m_queryNorm, m_rank).
  // It's possible when same locality was matched by different tokens.
  unordered_set<uint32_t> seen;
  auto it = els.begin();
  for (; it != els.end() && seen.size() < limitUniqueIds; ++it)
    seen.insert(it->GetId());

  els.erase(it, els.end());
}

void LocalityScorer::GroupBySimilarityAndOther(vector<ExLocality> & els) const
{
  sort(els.begin(), els.end(), [](ExLocality const & lhs, ExLocality const & rhs)
  {
    if (lhs.m_similarity != rhs.m_similarity)
      return lhs.m_similarity > rhs.m_similarity;
    if (lhs.m_locality.m_tokenRange.Size() != rhs.m_locality.m_tokenRange.Size())
      return lhs.m_locality.m_tokenRange.Size() > rhs.m_locality.m_tokenRange.Size();
    if (lhs.m_belongsToMatchedRegion != rhs.m_belongsToMatchedRegion)
      return lhs.m_belongsToMatchedRegion;
    if (lhs.m_rank != rhs.m_rank)
      return lhs.m_rank > rhs.m_rank;
    return lhs.m_locality.m_featureId < rhs.m_locality.m_featureId;
  });

  auto const lessDistance = [](ExLocality const & lhs, ExLocality const & rhs)
  { return lhs.m_distanceToPivot < rhs.m_distanceToPivot; };

  auto const compareSimilaritySizeAndRegion = [](ExLocality const & lhs, ExLocality const & rhs)
  {
    if (lhs.m_similarity != rhs.m_similarity)
      return lhs.m_similarity > rhs.m_similarity;
    if (lhs.m_locality.m_tokenRange.Size() != rhs.m_locality.m_tokenRange.Size())
      return lhs.m_locality.m_tokenRange.Size() > rhs.m_locality.m_tokenRange.Size();
    if (lhs.m_belongsToMatchedRegion != rhs.m_belongsToMatchedRegion)
      return lhs.m_belongsToMatchedRegion;
    return false;
  };

  vector<ExLocality> tmp;
  tmp.reserve(els.size());
  auto begin = els.begin();
  auto const end = els.end();
  while (begin != end)
  {
    // We can split els to equal ranges by similarity and size because we sorted els by similarity
    // size and region first.
    auto const range = equal_range(begin, end, *begin, compareSimilaritySizeAndRegion);
    auto const closest = min_element(range.first, range.second, lessDistance);
    tmp.emplace_back(std::move(*closest));
    for (auto it = range.first; it != range.second; ++it)
      if (it != closest)
        tmp.emplace_back(std::move(*it));
    begin = range.second;
  }

  els.swap(tmp);
}

void LocalityScorer::GetDocVecs(uint32_t localityId, vector<DocVec> & dvs) const
{
  vector<string> names;
  m_delegate.GetNames(localityId, names);

  for (auto const & name : names)
  {
    DocVec::Builder builder;
    ForEachNormalizedToken(name, [&](strings::UniString const & token)
    {
      if (!IsStopWord(token))
        builder.Add(token);
    });
    dvs.emplace_back(std::move(builder));
  }
}

double LocalityScorer::GetSimilarity(QueryVec & qv, IdfMap & docIdfs, vector<DocVec> & dvc) const
{
  double const kScale = 1e6;

  double similarity = 0;
  for (auto & dv : dvc)
    similarity = max(similarity, qv.Similarity(docIdfs, dv));

  // We need to scale similarity here to prevent floating-point
  // artifacts, and to make sorting by similarity more robust, as 1e-6
  // is good enough for our purposes.
  return round(similarity * kScale);
}

string DebugPrint(LocalityScorer::ExLocality const & el)
{
  ostringstream os;
  os << "LocalityScorer::ExLocality [ ";
  os << "m_locality=" << DebugPrint(el.m_locality) << ", ";
  os << "m_queryNorm=" << el.m_queryNorm << ", ";
  os << "m_similarity=" << el.m_similarity << ", ";
  os << "m_rank=" << static_cast<uint32_t>(el.m_rank);
  os << " ]";
  return os.str();
}
}  // namespace search
