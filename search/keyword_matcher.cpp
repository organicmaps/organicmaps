#include "keyword_matcher.hpp"
#include "../indexer/search_delimiters.hpp"
#include "../indexer/search_string_utils.hpp"
#include "../base/stl_add.hpp"
#include "../base/string_utils.hpp"
#include "../std/algorithm.hpp"

namespace search
{

KeywordMatcher::KeywordMatcher(strings::UniString const * const * pKeywords,
                               size_t keywordCount,
                               strings::UniString const * pPrefix)
  : m_pKeywords(pKeywords), m_keywordCount(keywordCount), m_pPrefix(pPrefix), m_bOwnKeywords(false)
{
  Initialize();
}

KeywordMatcher::KeywordMatcher(strings::UniString const * keywords,
                               size_t keywordCount,
                               strings::UniString const * pPrefix)
  : m_keywordCount(keywordCount), m_pPrefix(pPrefix), m_bOwnKeywords(false)
{
  Initialize();
  if (m_keywordCount > 0)
  {
    strings::UniString const * * pKeywords = new strings::UniString const * [m_keywordCount];
    for (size_t i = 0; i < m_keywordCount; ++i)
      pKeywords[i] = &keywords[i];
    m_bOwnKeywords = true;
    m_pKeywords = pKeywords;
  }
}

void KeywordMatcher::Initialize()
{
  ASSERT_LESS(m_keywordCount, size_t(MAX_TOKENS), ());
  m_keywordCount = min(m_keywordCount, size_t(MAX_TOKENS));
  if (m_pPrefix && m_pPrefix->empty())
    m_pPrefix = NULL;
}

KeywordMatcher::~KeywordMatcher()
{
  if (m_bOwnKeywords)
    delete [] m_pKeywords;
}

uint32_t KeywordMatcher::Score(string const & name) const
{
  return Score(NormalizeAndSimplifyString(name));
}

uint32_t KeywordMatcher::Score(strings::UniString const & name) const
{
  buffer_vector<strings::UniString, MAX_TOKENS> tokens;
  SplitUniString(name, MakeBackInsertFunctor(tokens), Delimiters());
  ASSERT_LESS(tokens.size(), size_t(MAX_TOKENS), ());
  return Score(tokens.data(), static_cast<int>(tokens.size()));
}

uint32_t KeywordMatcher::Score(strings::UniString const * tokens, int tokenCount) const
{
  ASSERT_LESS(tokenCount, int(MAX_TOKENS), ());

  // We will use this for scoring.
  unsigned char isTokenMatched[MAX_TOKENS] = { 0 };

  // Check that all keywords matched.
  for (int k = 0; k < m_keywordCount; ++k)
  {
    unsigned char isKeywordMatched = 0;
    for (int t = 0; t < tokenCount; ++t)
      if (*m_pKeywords[k] == tokens[t])
        isKeywordMatched = isTokenMatched[t] = 1;

    // All keywords should be matched.
    if (!isKeywordMatched)
      return MAX_SCORE;
  }

  // Check that prefix matched.
  if (m_pPrefix)
  {
    bool bPrefixMatched = false;
    for (int t = 0; t < tokenCount && !bPrefixMatched; ++t)
      if (StartsWith(tokens[t].begin(), tokens[t].end(),
                     m_pPrefix->begin(), m_pPrefix->end()))
        bPrefixMatched = true;
    if (!bPrefixMatched)
      return MAX_SCORE;
  }

  // Calculate score.
  int lastTokenMatched = 0;
  for (int t = 0; t < tokenCount; ++t)
    if (isTokenMatched[t])
      lastTokenMatched = t;
  uint32_t score = 0;
  for (int t = 0; t <= lastTokenMatched; ++t)
    if (tokens[t].size() > 2 && !isTokenMatched[t])
      ++score;

  return score;
}

}  // namespace search
