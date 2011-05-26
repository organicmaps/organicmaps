#include "keyword_matcher.hpp"
#include "delimiters.hpp"
#include "../base/string_utils.hpp"
#include "../std/numeric.hpp"

namespace search
{
namespace impl
{

KeywordMatcher::KeywordMatcher(strings::UniString * pKeywords,
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
    m_minPrefixMatchCost(m_maxPrefixMatchCost + 1)
{
}

void KeywordMatcher::ProcessName(string const & name)
{
  search::Delimiters delims;
  for (strings::TokenizeIterator<search::Delimiters> iter(name, delims); iter; ++iter)
  {
    strings::UniString const s = strings::MakeLowerCase(iter.GetUniString());

    for (size_t i = 0; i < m_minKeywordMatchCost.size(); ++i)
    {
      m_minKeywordMatchCost[i] = min(m_minKeywordMatchCost[i],
                                     m_keywordMatchFn(&m_pKewords[i][0], m_pKewords[i].size(),
                                                      &s[0], s.size(),
                                                      m_minKeywordMatchCost[i]));
    }

    if (!m_prefix.empty())
    {
      m_minPrefixMatchCost = min(m_minPrefixMatchCost,
                                 m_prefixMatchFn(&m_prefix[0], m_prefix.size(),
                                                 &s[0], s.size(),
                                                 m_minPrefixMatchCost));
    }
    else
      m_minPrefixMatchCost = 0;
  }
}

uint32_t KeywordMatcher::GetMatchScore() const
{
  return accumulate(m_minKeywordMatchCost.begin(), m_minKeywordMatchCost.end(),
                    m_minPrefixMatchCost);
}

}  // namespace search::impl
}  // namespace search
