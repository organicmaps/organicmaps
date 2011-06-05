#include "query.hpp"
#include "delimiters.hpp"
#include "latlon_match.hpp"
#include "string_match.hpp"
#include "../indexer/feature_visibility.hpp"
#include "../indexer/mercator.hpp"
#include "../base/stl_add.hpp"

namespace search
{
namespace impl
{
namespace
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

inline uint32_t GetMaxKeywordMatchScore() { return 512; }
inline uint32_t GetMaxPrefixMatchScore(int size)
{
  return min(512, 256 * max(0, size - 1));
}

inline KeywordMatcher MakeMatcher(vector<strings::UniString> const & tokens,
                                  strings::UniString const & prefix)
{
  return KeywordMatcher(tokens.empty() ? NULL : &tokens[0], tokens.size(),
                        prefix,
                        GetMaxKeywordMatchScore(), GetMaxPrefixMatchScore(prefix.size()),
                        &KeywordMatch, &PrefixMatch);
}

struct FeatureProcessor
{
  Query & m_query;

  explicit FeatureProcessor(Query & query) : m_query(query) {}

  void operator () (FeatureType const & feature) const
  {
    KeywordMatcher matcher(MakeMatcher(m_query.GetKeywords(), m_query.GetPrefix()));
    feature.ForEachNameRef(matcher);
    if (matcher.GetPrefixMatchScore() <= GetMaxPrefixMatchScore(m_query.GetPrefix().size()))
    {
      uint32_t const matchScore = matcher.GetMatchScore();
      if (matchScore <= GetMaxKeywordMatchScore())
      {
        pair<int, int> const scaleRange = feature::DrawableScaleRangeForText(feature);
        if (scaleRange.first < 0)
          return;

        m_query.AddResult(IntermediateResult(m_query.GetViewport(),
                                             feature,
                                             matcher.GetBestPrefixMatch(),
                                             matchScore,
                                             scaleRange.first));
      }
    }
  }
};

}  // unnamed namespace

Query::Query(string const & query, m2::RectD const & viewport, IndexType const * pIndex)
  : m_queryText(query), m_viewport(viewport), m_pIndex(pIndex ? new IndexType(*pIndex) : NULL),
    m_bTerminate(false)
{
  search::Delimiters delims;
  SplitAndNormalizeAndSimplifyString(query, MakeBackInsertFunctor(m_keywords), delims);
  if (!m_keywords.empty() && !delims(strings::LastUniChar(query)))
  {
    m_prefix.swap(m_keywords.back());
    m_keywords.pop_back();
  }
}

void Query::Search(function<void (Result const &)> const & f)
{
  // Lat lon matching.
  {
    double lat, lon, latPrec, lonPrec;
    if (search::MatchLatLon(m_queryText, lat, lon, latPrec, lonPrec))
    {
      double const precision = 5.0 * max(0.0001, min(latPrec, lonPrec));  // Min 55 meters
      m2::RectD const rect(MercatorBounds::LonToX(lon - precision),
                           MercatorBounds::LatToY(lat - precision),
                           MercatorBounds::LonToX(lon + precision),
                           MercatorBounds::LatToY(lat + precision));
      f(Result("(" + strings::to_string(lat) + ", " + strings::to_string(lon) + ")",
               0, rect,
               IntermediateResult::ResultDistance(m_viewport.Center(), rect.Center()),
               IntermediateResult::ResultDirection(m_viewport.Center(), rect.Center())));
    }
  }

  if (m_bTerminate)
    return;

  // Category matching.
  if (!m_prefix.empty())
  {
    KeywordMatcher matcher = MakeMatcher(vector<strings::UniString>(), m_prefix);
    LOG(LINFO, (m_prefix));
    matcher.ProcessNameToken("", strings::MakeUniString("restaurant"));
    uint32_t const matchScore = matcher.GetMatchScore();
    if (matcher.GetPrefixMatchScore() <= GetMaxPrefixMatchScore(m_prefix.size()) &&
        matchScore <= GetMaxKeywordMatchScore())
    {
      f(Result("restaurant", "restaurant "));
    }
  }

  if (m_bTerminate)
    return;

  // Feature matching.
  FeatureProcessor featureProcessor(*this);
  int const scale = scales::GetScaleLevel(m_viewport) + 1;
  if (scale > scales::GetUpperWorldScale())
  {
    m_pIndex->ForEachInRect(featureProcessor, m_viewport, scales::GetUpperWorldScale());
    if (m_bTerminate)
      return;
  }
  m_pIndex->ForEachInRect(featureProcessor, m_viewport, min(scales::GetUpperScale(), scale));
  if (m_bTerminate)
    return;

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

void Query::SearchAndDestroy(function<void (const Result &)> const & f)
{
  Search(f);
  delete this;
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
