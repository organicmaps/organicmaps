#include "query.hpp"
#include "categories_holder.hpp"
#include "delimiters.hpp"
#include "latlon_match.hpp"
#include "string_match.hpp"
#include "../indexer/feature_visibility.hpp"
#include "../base/exception.hpp"
#include "../base/stl_add.hpp"
#include "../std/scoped_ptr.hpp"

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
  if (sizeA != sizeB)
    return maxCost + 1;
  for (uint32_t i = 0; i< sizeA; ++i)
    if (sA[i] != sB[i])
      return maxCost + 1;
  return 0;
  // return StringMatchCost(sA, sizeA, sB, sizeB, DefaultMatchCost(), maxCost, false);
}

uint32_t PrefixMatch(strings::UniChar const * sA, uint32_t sizeA,
                     strings::UniChar const * sB, uint32_t sizeB,
                     uint32_t maxCost)
{
  if (sizeA > sizeB)
    return maxCost + 1;
  for (uint32_t i = 0; i< sizeA; ++i)
    if (sA[i] != sB[i])
      return maxCost + 1;
  return 0;
  // return StringMatchCost(sA, sizeA, sB, sizeB, DefaultMatchCost(), maxCost, true);
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
  DECLARE_EXCEPTION(StopException, RootException);

  Query & m_query;

  explicit FeatureProcessor(Query & query) : m_query(query) {}

  void operator () (FeatureType const & feature) const
  {
    if (m_query.GetTerminateFlag())
    {
      LOG(LDEBUG, ("Found terminate search flag", m_query.GetQueryText(), m_query.GetViewport()));
      MYTHROW(StopException, ());
    }

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
                                             matcher.GetBestMatchName(),
                                             matchScore,
                                             scaleRange.first));
      }
    }
  }
};

}  // unnamed namespace

Query::Query(string const & query, m2::RectD const & viewport, IndexType const * pIndex,
             Engine * pEngine, CategoriesHolder * pCategories)
  : m_queryText(query), m_viewport(viewport), m_pCategories(pCategories),
    m_pIndex(pIndex ? new IndexType(*pIndex) : NULL),
    m_resultsRemaining(10),
    m_pEngine(pEngine), m_bTerminate(false)
{
  search::Delimiters delims;
  SplitAndNormalizeAndSimplifyString(query, MakeBackInsertFunctor(m_keywords), delims);
  if (!m_keywords.empty() && !delims(strings::LastUniChar(query)))
  {
    m_prefix.swap(m_keywords.back());
    m_keywords.pop_back();
  }
}

Query::~Query()
{
  if (m_pEngine)
    m_pEngine->OnQueryDelete(this);
}

void Query::Search(function<void (Result const &)> const & f)
{
  // Lat lon matching.
  {
    double lat, lon, latPrec, lonPrec;
    if (search::MatchLatLon(m_queryText, lat, lon, latPrec, lonPrec))
    {
      double const precision = 5.0 * max(0.0001, min(latPrec, lonPrec));  // Min 55 meters
      AddResult(IntermediateResult(m_viewport, lat, lon, precision));
    }
  }

  if (m_bTerminate)
    return;

  // Category matching.
  if (m_pCategories)
  {
    for (int i = 0; i < m_keywords.size(); ++i)
    {

    }

    // TODO: Check if some keyword matched category?
    if (!m_prefix.empty())
    {
      for (CategoriesHolder::const_iterator iCategory = m_pCategories->begin();
           iCategory != m_pCategories->end(); ++iCategory)
      {
        KeywordMatcher matcher = MakeMatcher(vector<strings::UniString>(), m_prefix);

        for (vector<Category::Name>::const_iterator iName = iCategory->m_synonyms.begin();
             iName != iCategory->m_synonyms.end(); ++iName)
        {
          if (m_prefix.size() >= iName->m_prefixLengthToSuggest)
            matcher.ProcessNameToken(iName->m_Name, strings::MakeUniString(iName->m_Name));
        }

        if (matcher.GetPrefixMatchScore() <= GetMaxPrefixMatchScore(m_prefix.size()) &&
            matcher.GetMatchScore() <= GetMaxKeywordMatchScore())
        {
          int minPrefixMatchLength = 0;
          for (vector<Category::Name>::const_iterator iName = iCategory->m_synonyms.begin();
               iName != iCategory->m_synonyms.end(); ++iName)
            if (iName->m_Name == matcher.GetBestMatchName())
              minPrefixMatchLength = iName->m_prefixLengthToSuggest;

          AddResult(IntermediateResult(matcher.GetBestMatchName(),
                                       matcher.GetBestMatchName() + ' ',
                                       minPrefixMatchLength));
        }
      }
    }
  }

  if (m_bTerminate)
    return;

  FlushResults(f);

  // Feature matching.
  FeatureProcessor featureProcessor(*this);
  int const scale = scales::GetScaleLevel(m_viewport) + 1;

  try
  {
    m_pIndex->ForEachInRect(featureProcessor,
                            m2::RectD(MercatorBounds::minX, MercatorBounds::minY,
                                      MercatorBounds::maxX, MercatorBounds::maxY),
                            // m_viewport,
                            scales::GetUpperWorldScale());
  }
  catch (FeatureProcessor::StopException &)
  {
    LOG(LDEBUG, ("FeatureProcessor::StopException"));
  }
  if (m_bTerminate)
    return;

  if (scale > scales::GetUpperWorldScale())
  {
    try
    {
      m_pIndex->ForEachInRect(featureProcessor, m_viewport, min(scales::GetUpperScale(), scale));
    }
    catch (FeatureProcessor::StopException &)
    {
      LOG(LDEBUG, ("FeatureProcessor::StopException"));
    }
  }
  if (m_bTerminate)
    return;

  FlushResults(f);
  f(Result(string(), string()));  // Send last search result marker.
}

void Query::FlushResults(const function<void (const Result &)> &f)
{
  vector<Result> results;
  results.reserve(m_results.size());
  while (!m_results.empty())
  {
    results.push_back(m_results.top().GenerateFinalResult());
    m_results.pop();
  }
  for (vector<Result>::const_reverse_iterator it = results.rbegin(); it != results.rend(); ++it)
    f(*it);
  m_resultsRemaining = max(0, m_resultsRemaining - static_cast<int>(results.size()));
}

void Query::SearchAndDestroy(function<void (const Result &)> const & f)
{
  scoped_ptr<Query> scopedThis(this);
  UNUSED_VALUE(scopedThis);
  Search(f);
}

void Query::AddResult(IntermediateResult const & result)
{
  if (m_results.size() < m_resultsRemaining)
    m_results.push(result);
  else if (result < m_results.top())
  {
    m_results.pop();
    m_results.push(result);
  }
}

}  // namespace search::impl
}  // namespace search
