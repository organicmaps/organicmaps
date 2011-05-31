#include "query.hpp"
#include "delimiters.hpp"
#include "keyword_matcher.hpp"
#include "string_match.hpp"
#include "../indexer/feature_visibility.hpp"
#include "../base/stl_add.hpp"

namespace search
{
namespace impl
{

uint32_t KeywordMatch(strings::UniChar const * sA, uint32_t sizeA,
                      strings::UniChar const * sB, uint32_t sizeB,
                      uint32_t maxCost)
{
  /*
  if (sizeA != sizeB)
    return maxCost + 1;
  for (uint32_t i = 0; i< sizeA; ++i)
    if (sA[i] != sB[i])
      return maxCost + 1;
  return 0;
  */
  return StringMatchCost(sA, sizeA, sB, sizeB, DefaultMatchCost(), maxCost, false);
}

uint32_t PrefixMatch(strings::UniChar const * sA, uint32_t sizeA,
                     strings::UniChar const * sB, uint32_t sizeB,
                     uint32_t maxCost)
{
  /*
  if (sizeA > sizeB)
    return maxCost + 1;
  for (uint32_t i = 0; i< sizeA; ++i)
    if (sA[i] != sB[i])
      return maxCost + 1;
  return 0;
  */
  return StringMatchCost(sA, sizeA, sB, sizeB, DefaultMatchCost(), maxCost, true);
}


Query::Query(string const & query, m2::RectD const & rect, IndexType const * pIndex)
  : m_queryText(query), m_rect(rect), m_pIndex(pIndex)
{
  search::Delimiters delims;
  SplitAndNormalizeAndSimplifyString(query, MakeBackInsertFunctor(m_keywords), delims);
  if (!m_keywords.empty() && !delims(strings::LastUniChar(query)))
  {
    m_prefix.swap(m_keywords.back());
    m_keywords.pop_back();
  }
}

struct FeatureProcessor
{
  Query & m_query;

  explicit FeatureProcessor(Query & query) : m_query(query) {}

  void operator () (FeatureType const & feature) const
  {
    uint32_t const maxKeywordMatchScore = 512;
    uint32_t const maxPrefixMatchScore = min(static_cast<int>(maxKeywordMatchScore),
                                             256 * max(0, int(m_query.m_prefix.size()) - 1));
    size_t const kwSize = m_query.m_keywords.size();
    KeywordMatcher matcher(kwSize ? &m_query.m_keywords[0] : NULL, kwSize, m_query.m_prefix,
                           maxKeywordMatchScore, maxPrefixMatchScore,
                           &KeywordMatch, &PrefixMatch);
    feature.ForEachNameRef(matcher);
    if (matcher.GetPrefixMatchScore() <= maxPrefixMatchScore)
    {
      uint32_t const matchScore = matcher.GetMatchScore();
      if (matchScore <= maxKeywordMatchScore)
      {
        int const minVisibleScale = feature::MinDrawableScaleForText(feature);
        // if (minVisibleScale < 0)
        //  return;

        m_query.AddResult(IntermediateResult(feature, matcher.GetBestPrefixMatch(),
                                             matchScore, minVisibleScale));
      }
    }
  }
};

void Query::Search(function<void (Result const &)> const & f)
{
  FeatureProcessor featureProcessor(*this);
  int const scale = scales::GetScaleLevel(m_rect) + 1;
  if (scale > scales::GetUpperWorldScale())
    m_pIndex->ForEachInRect(featureProcessor, m_rect, scales::GetUpperWorldScale());
  m_pIndex->ForEachInRect(featureProcessor, m_rect, min(scales::GetUpperScale(), scale));

  vector<Result> results;
  results.reserve(m_results.size());
  while (!m_results.empty())
  {
    results.push_back(m_results.top().GenerateFinalResult());
    m_results.pop();
  }
  for (vector<Result>::const_reverse_iterator it = results.rbegin(); it != results.rend(); ++it)
    f(*it);
}

void Query::AddResult(IntermediateResult const & result)
{
  if (m_results.size() < 10)
    m_results.push(result);
  else if (result < m_results.top())
  {
    m_results.pop();
    m_results.push(result);
  }
}

}  // namespace search::impl
}  // namespace search
