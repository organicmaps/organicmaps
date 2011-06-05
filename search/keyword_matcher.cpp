#include "keyword_matcher.hpp"
#include "delimiters.hpp"
#include "string_match.hpp"
#include "../base/logging.hpp"
#include "../base/string_utils.hpp"
#include "../std/bind.hpp"
#include "../std/numeric.hpp"

namespace search
{
namespace impl
{

KeywordMatcher::KeywordMatcher(strings::UniString const * pKeywords,
                               size_t keywordsCount,
                               strings::UniString const & prefix,
                               uint32_t maxKeywordMatchCost, uint32_t maxPrefixMatchCost,
                               StringMatchFn keywordMatchFn, StringMatchFn prefixMatchFn)
  : m_pKewords(pKeywords), m_prefix(prefix),
    m_maxKeywordMatchCost(maxKeywordMatchCost),
    m_maxPrefixMatchCost(maxPrefixMatchCost),
    m_keywordMatchFn(keywordMatchFn),
    m_prefixMatchFn(prefixMatchFn),
    m_minKeywordMatchCost(keywordsCount, m_maxKeywordMatchCost + 1),
    m_minPrefixMatchCost(m_maxPrefixMatchCost + 1),
    m_bestMatchTokenCount(0)
{
}

void KeywordMatcher::ProcessName(string const & name)
{
  SplitAndNormalizeAndSimplifyString(
        name, bind(&KeywordMatcher::ProcessNameToken, this, cref(name), _1), Delimiters());
}

void KeywordMatcher::ProcessNameToken(string const & name, strings::UniString const & s)
{
  uint32_t tokensMatched = 0;
  for (size_t i = 0; i < m_minKeywordMatchCost.size(); ++i)
  {
    uint32_t const matchCost = m_keywordMatchFn(&m_pKewords[i][0], m_pKewords[i].size(),
                                                &s[0], s.size(), m_minKeywordMatchCost[i]);
    if (matchCost <= m_maxKeywordMatchCost)
    {
      ++tokensMatched;
      if (matchCost < m_minKeywordMatchCost[i])
      {
        // LOG(LDEBUG, (matchCost, name));
        m_minKeywordMatchCost[i] = matchCost;
      }
    }
  }

  if (!m_prefix.empty())
  {
    uint32_t const matchCost = m_prefixMatchFn(&m_prefix[0], m_prefix.size(),
                                               &s[0], s.size(), m_minPrefixMatchCost);
    if (matchCost <= m_maxPrefixMatchCost)
    {
      ++tokensMatched;
      if (matchCost < m_minPrefixMatchCost)
        m_minPrefixMatchCost = matchCost;
    }
  }
  else
  {
    ++tokensMatched;
    m_minPrefixMatchCost = 0;
  }

  if (tokensMatched > m_bestMatchTokenCount)
  {
    m_bestMatchName = name;
    m_bestMatchTokenCount = tokensMatched;
  }
}

uint32_t KeywordMatcher::GetMatchScore() const
{
  return accumulate(m_minKeywordMatchCost.begin(), m_minKeywordMatchCost.end(),
                    m_minPrefixMatchCost);
}

}  // namespace search::impl
}  // namespace search
