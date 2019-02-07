#include "search/locality_scorer.hpp"

#include "search/cbv.hpp"
#include "search/geocoder_context.hpp"
#include "search/idf_map.hpp"
#include "search/token_slice.hpp"
#include "search/utils.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/checked_cast.hpp"
#include "base/dfa_helpers.hpp"
#include "base/levenshtein_dfa.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <utility>

using namespace std;
using namespace strings;

namespace search
{
namespace
{
class IdfMapDelegate : public IdfMap::Delegate
{
public:
  IdfMapDelegate(vector<pair<LevenshteinDFA, uint64_t>> const & tokensToDf,
                 vector<pair<PrefixDFAModifier<LevenshteinDFA>, uint64_t>> const & prefixToDf)
    : m_tokensToDf(tokensToDf), m_prefixToDf(prefixToDf)
  {
  }

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
  vector<pair<PrefixDFAModifier<LevenshteinDFA>, uint64_t>> const & m_prefixToDf;
};
}  // namespace

// static
size_t const LocalityScorer::kDefaultReadLimit = 100;

// LocalityScorer::ExLocality ----------------------------------------------------------------------
LocalityScorer::ExLocality::ExLocality(Locality const & locality, double queryNorm, uint8_t rank)
  : m_locality(locality), m_queryNorm(queryNorm), m_rank(rank)
{
}

// LocalityScorer ----------------------------------------------------------------------------------
LocalityScorer::LocalityScorer(QueryParams const & params, Delegate const & delegate)
  : m_params(params), m_delegate(delegate)
{
}

void LocalityScorer::GetTopLocalities(MwmSet::MwmId const & countryId, BaseContext const & ctx,
                                      CBV const & filter, size_t limit,
                                      vector<Locality> & localities) const
{
  double const kUnknownIdf = 1.0;

  CHECK_EQUAL(ctx.m_numTokens, m_params.GetNumTokens(), ());

  localities.clear();

  vector<CBV> intersections(ctx.m_numTokens);
  vector<pair<LevenshteinDFA, uint64_t>> tokensToDf;
  vector<pair<PrefixDFAModifier<LevenshteinDFA>, uint64_t>> prefixToDf;
  bool const havePrefix = ctx.m_numTokens > 0 && m_params.LastTokenIsPrefix();
  size_t const nonPrefixTokens = havePrefix ? ctx.m_numTokens - 1 : ctx.m_numTokens;
  for (size_t i = 0; i < nonPrefixTokens; ++i)
  {
    intersections[i] = filter.Intersect(ctx.m_features[i]);
    auto const df = intersections.back().PopCount();
    if (df != 0)
    {
      m_params.GetToken(i).ForEach([&tokensToDf, &df](UniString const & s) {
        tokensToDf.emplace_back(BuildLevenshteinDFA(s), df);
      });
    }
  }

  if (havePrefix)
  {
    auto const count = ctx.m_numTokens - 1;
    intersections[count] = filter.Intersect(ctx.m_features[count]);
    auto const prefixDf = intersections.back().PopCount();
    if (prefixDf != 0)
    {
      m_params.GetToken(count).ForEach([&prefixToDf, &prefixDf](UniString const & s) {
        prefixToDf.emplace_back(PrefixDFAModifier<LevenshteinDFA>(BuildLevenshteinDFA(s)),
                                prefixDf);
      });
    }
  }

  IdfMapDelegate delegate(tokensToDf, prefixToDf);
  IdfMap idfs(delegate, kUnknownIdf);

  for (size_t startToken = 0; startToken < ctx.m_numTokens; ++startToken)
  {
    auto intersection = intersections[startToken];
    QueryVec::Builder builder;

    for (size_t endToken = startToken + 1; endToken <= ctx.m_numTokens && !intersection.IsEmpty();
         ++endToken)
    {
      auto const curToken = endToken - 1;
      auto const & token = m_params.GetToken(curToken).m_original;
      if (m_params.IsPrefixToken(curToken))
        builder.SetPrefix(token);
      else
        builder.AddFull(token);

      TokenRange const tokenRange(startToken, endToken);
      // Skip locality candidates that match only numbers.
      if (!m_params.IsNumberTokens(tokenRange))
      {
        intersection.ForEach([&](uint64_t bit) {
          auto const featureId = base::asserted_cast<uint32_t>(bit);
          localities.emplace_back(countryId, featureId, tokenRange, QueryVec(idfs, builder));
        });
      }

      if (endToken < ctx.m_numTokens)
        intersection = intersection.Intersect(intersections[endToken]);
    }
  }

  LeaveTopLocalities(idfs, limit, localities);
}

void LocalityScorer::LeaveTopLocalities(IdfMap & idfs, size_t limit,
                                        vector<Locality> & localities) const
{
  vector<ExLocality> els;
  els.reserve(localities.size());
  for (auto & locality : localities)
  {
    auto const queryNorm = locality.m_queryVec.Norm();
    auto const rank = m_delegate.GetRank(locality.m_featureId);
    els.emplace_back(locality, queryNorm, rank);
  }

  // We don't want to read too many names for localities, so this is
  // the best effort - select the best features by available params -
  // query norm and rank.
  LeaveTopByNormAndRank(max(limit, kDefaultReadLimit) /* limitUniqueIds */, els);

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
    for (; i < j; ++i)
      els[i].m_similarity = GetSimilarity(els[i].m_locality.m_queryVec, idfs, dvs);
  }

  LeaveTopBySimilarityAndRank(limit, els);
  ASSERT_LESS_OR_EQUAL(els.size(), limit, ());

  localities.clear();
  localities.reserve(els.size());
  for (auto const & el : els)
    localities.push_back(el.m_locality);
  ASSERT_LESS_OR_EQUAL(localities.size(), limit, ());
}

void LocalityScorer::LeaveTopByNormAndRank(size_t limitUniqueIds, vector<ExLocality> & els) const
{
  sort(els.begin(), els.end(), [](ExLocality const & lhs, ExLocality const & rhs) {
    auto const ln = lhs.m_queryNorm;
    auto const rn = rhs.m_queryNorm;
    if (ln != rn)
      return ln > rn;
    return lhs.m_rank > rhs.m_rank;
  });

  unordered_set<uint32_t> seen;
  for (size_t i = 0; i < els.size() && seen.size() < limitUniqueIds; ++i)
    seen.insert(els[i].GetId());
  ASSERT_LESS_OR_EQUAL(seen.size(), limitUniqueIds, ());

  base::EraseIf(els, [&](ExLocality const & el) { return seen.find(el.GetId()) == seen.cend(); });
}

void LocalityScorer::LeaveTopBySimilarityAndRank(size_t limit, vector<ExLocality> & els) const
{
  sort(els.begin(), els.end(), [](ExLocality const & lhs, ExLocality const & rhs) {
    if (lhs.m_similarity != rhs.m_similarity)
      return lhs.m_similarity > rhs.m_similarity;
    if (lhs.m_rank != rhs.m_rank)
      return lhs.m_rank > rhs.m_rank;
    return lhs.m_locality.m_featureId < rhs.m_locality.m_featureId;
  });

  unordered_set<uint32_t> seen;

  size_t n = 0;
  for (size_t i = 0; i < els.size() && n < limit; ++i)
  {
    auto const id = els[i].GetId();
    if (seen.insert(id).second)
    {
      els[n] = els[i];
      ++n;
    }
  }
  els.erase(els.begin() + n, els.end());
}

void LocalityScorer::GetDocVecs(uint32_t localityId, vector<DocVec> & dvs) const
{
  vector<string> names;
  m_delegate.GetNames(localityId, names);

  for (auto const & name : names)
  {
    vector<UniString> tokens;
    NormalizeAndTokenizeString(name, tokens);

    DocVec::Builder builder;
    for (auto const & token : tokens)
      builder.Add(token);
    dvs.emplace_back(builder);
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
