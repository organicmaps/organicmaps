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
    KeywordMatcher matcher(MakeMatcher(m_query.m_keywords, m_query.m_prefix));
    feature.ForEachNameRef(matcher);
    if (matcher.GetPrefixMatchScore() <= GetMaxPrefixMatchScore(m_query.m_prefix.size()))
    {
      uint32_t const matchScore = matcher.GetMatchScore();
      if (matchScore <= GetMaxKeywordMatchScore())
      {
        int const minVisibleScale = feature::MinDrawableScaleForText(feature);
        if (minVisibleScale < 0)
          return;

        m_query.AddResult(IntermediateResult(feature, matcher.GetBestPrefixMatch(),
                                             matchScore, minVisibleScale));
      }
    }
  }
};

}  // unnamed namespace

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

void Query::Search(function<void (Result const &)> const & f)
{
  // Lat lon matching.
  {
    double lat, lon, latPrec, lonPrec;
    if (search::MatchLatLon(m_queryText, lat, lon, latPrec, lonPrec))
    {
      double const precision = 5.0 * max(0.0001, min(latPrec, lonPrec));  // Min 55 meters
      f(Result("(" + strings::to_string(lat) + ", " + strings::to_string(lon) + ")",
               0,
               m2::RectD(MercatorBounds::LonToX(lon - precision),
                         MercatorBounds::LatToY(lat - precision),
                         MercatorBounds::LonToX(lon + precision),
                         MercatorBounds::LatToY(lat + precision))));
    }
  }

  // Category matching.
  {
    KeywordMatcher matcher = MakeMatcher(m_keywords, m_prefix);
    matcher.ProcessNameToken("", strings::MakeUniString("restaurant"));
    uint32_t const matchScore = matcher.GetMatchScore();
    if (matcher.GetPrefixMatchScore() <= GetMaxPrefixMatchScore(m_prefix.size()) &&
        matchScore <= GetMaxKeywordMatchScore())
    {
      f(Result("restaurant", "restaurant "));
    }
  }

  // Feature matching.
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
