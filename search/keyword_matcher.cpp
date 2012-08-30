#include "keyword_matcher.hpp"

#include "../indexer/search_delimiters.hpp"
#include "../indexer/search_string_utils.hpp"

#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"


namespace search
{

void KeywordMatcher::SetKeywords(StringT const * keywords, size_t count, StringT const * prefix)
{
  ASSERT_LESS ( count, static_cast<size_t>(MAX_TOKENS), () );

  m_keywords.resize(count);
  for (size_t i = 0; i < count; ++i)
    m_keywords[i] = &keywords[i];

  m_prefix = prefix;
  if (m_prefix && m_prefix->empty())
    m_prefix = 0;
}

uint32_t KeywordMatcher::Score(string const & name) const
{
  return Score(NormalizeAndSimplifyString(name));
}

uint32_t KeywordMatcher::Score(StringT const & name) const
{
  buffer_vector<StringT, MAX_TOKENS> tokens;
  SplitUniString(name, MakeBackInsertFunctor(tokens), Delimiters());

  /// @todo Some Arabian names have a lot of tokens.
  /// Trim this stuff while generation.
  //ASSERT_LESS ( tokens.size(), static_cast<size_t>(MAX_TOKENS), () );

  return Score(tokens.data(), min(size_t(MAX_TOKENS-1), tokens.size()));
}

uint32_t KeywordMatcher::Score(StringT const * tokens, size_t count) const
{
  ASSERT_LESS ( count, static_cast<size_t>(MAX_TOKENS), () );

  // boolean array of matched input tokens
  unsigned char isTokenMatched[MAX_TOKENS] = { 0 };

  // calculate penalty by keywords - add MAX_TOKENS for each unmatched keyword
  uint32_t score = 0;
  for (size_t i = 0; i < m_keywords.size(); ++i)
  {
    unsigned char isKeywordMatched = 0;
    for (size_t j = 0; j < count; ++j)
      if (*m_keywords[i] == tokens[j])
        isKeywordMatched = isTokenMatched[j] = 1;

    if (!isKeywordMatched)
      score += MAX_TOKENS;
  }

  // calculate penalty for prefix - add MAX_TOKENS for unmatched prefix
  if (m_prefix)
  {
    bool bPrefixMatched = false;
    for (size_t i = 0; i < count && !bPrefixMatched; ++i)
      if (StartsWith(tokens[i].begin(), tokens[i].end(),
                     m_prefix->begin(), m_prefix->end()))
      {
        bPrefixMatched = true;
      }

    if (!bPrefixMatched)
      score += MAX_TOKENS;
  }

  // add penalty for each unmatched token in input sequence
  for (size_t i = 0; i <= count; ++i)
  {
    // check for token length (skip common tokens such as "de", "la", "a")
    if (tokens[i].size() > 2 && !isTokenMatched[i])
      ++score;
  }

  return score;
}

}  // namespace search
