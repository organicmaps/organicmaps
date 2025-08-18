#include "search/keyword_matcher.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/buffer_vector.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <sstream>

namespace search
{
using namespace std;

KeywordMatcher::KeywordMatcher()
{
  Clear();
}

void KeywordMatcher::Clear()
{
  m_keywords.clear();
  m_prefix.clear();
}

void KeywordMatcher::SetKeywords(QueryString const & query)
{
  m_keywords.assign(query.m_tokens.begin(), query.m_tokens.end());
  m_prefix = query.m_prefix;
}

KeywordMatcher::Score KeywordMatcher::CalcScore(string_view name) const
{
  return CalcScore(NormalizeAndSimplifyString(name));
}

KeywordMatcher::Score KeywordMatcher::CalcScore(strings::UniString const & name) const
{
  buffer_vector<strings::UniString, kMaxNumTokens> tokens;
  SplitUniString(name, base::MakeBackInsertFunctor(tokens), Delimiters());

  return CalcScore(tokens.data(), tokens.size());
}

KeywordMatcher::Score KeywordMatcher::CalcScore(strings::UniString const * tokens, size_t count) const
{
  // Some names can have too many tokens. Trim them.
  count = min(count, kMaxNumTokens);

  vector<bool> isQueryTokenMatched(m_keywords.size());
  vector<bool> isNameTokenMatched(count);
  uint32_t sumTokenMatchDistance = 0;
  int8_t prevTokenMatchDistance = 0;
  bool prefixMatched = true;

  for (size_t i = 0; i < m_keywords.size(); ++i)
  {
    for (size_t j = 0; j < count && !isQueryTokenMatched[i]; ++j)
    {
      if (!isNameTokenMatched[j] && m_keywords[i] == tokens[j])
      {
        isQueryTokenMatched[i] = isNameTokenMatched[j] = true;
        int8_t const tokenMatchDistance = i - j;
        sumTokenMatchDistance += abs(tokenMatchDistance - prevTokenMatchDistance);
        prevTokenMatchDistance = tokenMatchDistance;
      }
    }
  }

  if (!m_prefix.empty())
  {
    prefixMatched = false;
    for (size_t j = 0; j < count && !prefixMatched; ++j)
    {
      if (!isNameTokenMatched[j] &&
          strings::StartsWith(tokens[j].begin(), tokens[j].end(), m_prefix.begin(), m_prefix.end()))
      {
        isNameTokenMatched[j] = prefixMatched = true;
        int8_t const tokenMatchDistance = int(m_keywords.size()) - j;
        sumTokenMatchDistance += abs(tokenMatchDistance - prevTokenMatchDistance);
      }
    }
  }

  uint8_t numQueryTokensMatched = 0;
  for (size_t i = 0; i < isQueryTokenMatched.size(); ++i)
    if (isQueryTokenMatched[i])
      ++numQueryTokensMatched;

  Score score;
  score.m_fullQueryMatched = prefixMatched && (numQueryTokensMatched == isQueryTokenMatched.size());
  score.m_prefixMatched = prefixMatched;
  score.m_numQueryTokensAndPrefixMatched = numQueryTokensMatched + (prefixMatched ? 1 : 0);

  score.m_nameTokensMatched = 0;
  score.m_nameTokensLength = 0;
  for (size_t i = 0; i < count; ++i)
  {
    if (isNameTokenMatched[i])
      score.m_nameTokensMatched |= (1 << (kMaxNumTokens - 1 - i));
    score.m_nameTokensLength += tokens[i].size();
  }

  score.m_sumTokenMatchDistance = sumTokenMatchDistance;
  return score;
}

KeywordMatcher::Score::Score()
  : m_sumTokenMatchDistance(0)
  , m_nameTokensMatched(0)
  , m_nameTokensLength(0)
  , m_numQueryTokensAndPrefixMatched(0)
  , m_fullQueryMatched(false)
  , m_prefixMatched(false)
{}

bool KeywordMatcher::Score::operator<(KeywordMatcher::Score const & s) const
{
  if (m_fullQueryMatched != s.m_fullQueryMatched)
    return m_fullQueryMatched < s.m_fullQueryMatched;
  if (m_numQueryTokensAndPrefixMatched != s.m_numQueryTokensAndPrefixMatched)
    return m_numQueryTokensAndPrefixMatched < s.m_numQueryTokensAndPrefixMatched;
  if (m_prefixMatched != s.m_prefixMatched)
    return m_prefixMatched < s.m_prefixMatched;
  if (m_nameTokensMatched != s.m_nameTokensMatched)
    return m_nameTokensMatched < s.m_nameTokensMatched;
  if (m_sumTokenMatchDistance != s.m_sumTokenMatchDistance)
    return m_sumTokenMatchDistance > s.m_sumTokenMatchDistance;

  return false;
}

bool KeywordMatcher::Score::operator==(KeywordMatcher::Score const & s) const
{
  return m_sumTokenMatchDistance == s.m_sumTokenMatchDistance && m_nameTokensMatched == s.m_nameTokensMatched &&
         m_numQueryTokensAndPrefixMatched == s.m_numQueryTokensAndPrefixMatched &&
         m_fullQueryMatched == s.m_fullQueryMatched && m_prefixMatched == s.m_prefixMatched;
}

bool KeywordMatcher::Score::LessInTokensLength(Score const & s) const
{
  if (m_fullQueryMatched)
  {
    ASSERT(s.m_fullQueryMatched, ());
    return m_nameTokensLength > s.m_nameTokensLength;
  }
  return false;
}

string DebugPrint(KeywordMatcher::Score const & score)
{
  ostringstream out;
  out << "KeywordMatcher::Score(";
  out << "FQM=" << score.m_fullQueryMatched;
  out << ",nQTM=" << static_cast<int>(score.m_numQueryTokensAndPrefixMatched);
  out << ",PM=" << score.m_prefixMatched;
  out << ",NTM=";
  for (int i = static_cast<int>(kMaxNumTokens) - 1; i >= 0; --i)
    out << ((score.m_nameTokensMatched >> i) & 1);
  out << ",STMD=" << score.m_sumTokenMatchDistance;
  out << ")";
  return out.str();
}
}  // namespace search
