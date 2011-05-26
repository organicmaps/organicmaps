#include "query.hpp"
#include "delimiters.hpp"
#include "keyword_matcher.hpp"
#include "string_match.hpp"

namespace search
{
namespace impl
{

uint32_t KeywordMatch(strings::UniChar const * sA, uint32_t sizeA,
                      strings::UniChar const * sB, uint32_t sizeB,
                      uint32_t maxCost)
{
  return StringMatchCost(sA, sizeA, sB, sizeB, DefaultMatchCost(), maxCost, false);
}

uint32_t PrefixMatch(strings::UniChar const * sA, uint32_t sizeA,
                     strings::UniChar const * sB, uint32_t sizeB,
                     uint32_t maxCost)
{
  return StringMatchCost(sA, sizeA, sB, sizeB, DefaultMatchCost(), maxCost, true);
}


Query::Query(string const & query, m2::RectD const & rect, IndexType const * pIndex)
  : m_queryText(query), m_rect(rect), m_pIndex(pIndex)
{
  search::Delimiters delims;
  for (strings::TokenizeIterator<search::Delimiters> iter(query, delims); iter; ++iter)
  {
    if (iter.IsLast() && !delims(strings::LastUniChar(query)))
      m_prefix = strings::MakeLowerCase(iter.GetUniString());
    else
      m_keywords.push_back(strings::MakeLowerCase(iter.GetUniString()));
  }
}

struct FeatureProcessor
{
  Query & m_query;

  explicit FeatureProcessor(Query & query) : m_query(query) {}

  void operator () (FeatureType const & feature) const
  {
    KeywordMatcher matcher(&m_query.m_keywords[0], m_query.m_keywords.size(), m_query.m_prefix,
                           512, 256 * max(0, int(m_query.m_prefix.size()) - 1),
                           &KeywordMatch, &PrefixMatch);
    feature.ForEachNameRef(matcher);
    m_query.AddResult(Result(matcher.GetBestPrefixMatch(), feature.GetLimitRect(-1),
                             matcher.GetMatchScore()));
  }
};

void Query::Search(function<void (Result const &)> const & f)
{
  FeatureProcessor featureProcessor(*this);
  m_pIndex->ForEachInRect(featureProcessor, m_rect,
                          min(scales::GetUpperScale(), scales::GetScaleLevel(m_rect) + 1));

  vector<Result> results;
  results.reserve(m_resuts.size());
  while (!m_resuts.empty())
  {
    results.push_back(m_resuts.top());
    m_resuts.pop();
  }
  for (vector<Result>::const_reverse_iterator it = results.rbegin(); it != results.rend(); ++it)
    f(*it);
}

void Query::AddResult(Result const & result)
{
  m_resuts.push(result);
  while (m_resuts.size() > 10)
    m_resuts.pop();
}

bool Query::ResultBetter::operator ()(Result const & r1, Result const & r2) const
{
  return r1.GetPenalty() < r2.GetPenalty();
}

}  // namespace search::impl
}  // namespace search
