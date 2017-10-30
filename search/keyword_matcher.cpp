#include "search/keyword_matcher.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/sstream.hpp"

namespace search
{
KeywordMatcher::KeywordMatcher()
{
  Clear();
}

void KeywordMatcher::Clear()
{
  m_keywords.clear();
  m_prefix.clear();
}

void KeywordMatcher::SetKeywords(StringT const * keywords, size_t count, StringT const & prefix)
{
  m_keywords.assign(keywords, keywords + count);
  m_prefix = prefix;
}

KeywordMatcher::ScoreT KeywordMatcher::Score(string const & name) const
{
  return Score(NormalizeAndSimplifyString(name));
}

KeywordMatcher::ScoreT KeywordMatcher::Score(StringT const & name) const
{
  buffer_vector<StringT, MAX_TOKENS> tokens;
  SplitUniString(name, MakeBackInsertFunctor(tokens), Delimiters());

  return Score(tokens.data(), tokens.size());
}

KeywordMatcher::ScoreT KeywordMatcher::Score(StringT const * tokens, size_t count) const
{
  // Some names can have too many tokens. Trim them.
  count = min(count, static_cast<size_t>(MAX_TOKENS));

  vector<bool> isQueryTokenMatched(m_keywords.size());
  vector<bool> isNameTokenMatched(count);
  uint32_t sumTokenMatchDistance = 0;
  int8_t prevTokenMatchDistance = 0;
  bool bPrefixMatched = true;

  for (int i = 0; i < m_keywords.size(); ++i)
    for (int j = 0; j < count && !isQueryTokenMatched[i]; ++j)
      if (!isNameTokenMatched[j] && m_keywords[i] == tokens[j])
      {
        isQueryTokenMatched[i] = isNameTokenMatched[j] = true;
        int8_t const tokenMatchDistance = i - j;
        sumTokenMatchDistance += abs(tokenMatchDistance - prevTokenMatchDistance);
        prevTokenMatchDistance = tokenMatchDistance;
      }

  if (!m_prefix.empty())
  {
    bPrefixMatched = false;
    for (int j = 0; j < count && !bPrefixMatched; ++j)
      if (!isNameTokenMatched[j] &&
          strings::StartsWith(tokens[j].begin(), tokens[j].end(), m_prefix.begin(), m_prefix.end()))
      {
        isNameTokenMatched[j] = bPrefixMatched = true;
        int8_t const tokenMatchDistance = int(m_keywords.size()) - j;
        sumTokenMatchDistance += abs(tokenMatchDistance - prevTokenMatchDistance);
      }
  }

  uint8_t numQueryTokensMatched = 0;
  for (size_t i = 0; i < isQueryTokenMatched.size(); ++i)
    if (isQueryTokenMatched[i])
      ++numQueryTokensMatched;

  ScoreT score;
  score.m_bFullQueryMatched = bPrefixMatched && (numQueryTokensMatched == isQueryTokenMatched.size());
  score.m_bPrefixMatched = bPrefixMatched;
  score.m_numQueryTokensAndPrefixMatched = numQueryTokensMatched + (bPrefixMatched ? 1 : 0);

  score.m_nameTokensMatched = 0;
  score.m_nameTokensLength = 0;
  for (size_t i = 0; i < count; ++i)
  {
    if (isNameTokenMatched[i])
      score.m_nameTokensMatched |= (1 << (MAX_TOKENS-1 - i));
    score.m_nameTokensLength += tokens[i].size();
  }

  score.m_sumTokenMatchDistance = sumTokenMatchDistance;
  return score;
}

KeywordMatcher::ScoreT::ScoreT()
  : m_sumTokenMatchDistance(0), m_nameTokensMatched(0), m_nameTokensLength(0),
    m_numQueryTokensAndPrefixMatched(0), m_bFullQueryMatched(false), m_bPrefixMatched(false)
{
}

bool KeywordMatcher::ScoreT::operator<(KeywordMatcher::ScoreT const & s) const
{
  if (m_bFullQueryMatched != s.m_bFullQueryMatched)
    return m_bFullQueryMatched < s.m_bFullQueryMatched;
  if (m_numQueryTokensAndPrefixMatched != s.m_numQueryTokensAndPrefixMatched)
    return m_numQueryTokensAndPrefixMatched < s.m_numQueryTokensAndPrefixMatched;
  if (m_bPrefixMatched != s.m_bPrefixMatched)
    return m_bPrefixMatched < s.m_bPrefixMatched;
  if (m_nameTokensMatched != s.m_nameTokensMatched)
    return m_nameTokensMatched < s.m_nameTokensMatched;
  if (m_sumTokenMatchDistance != s.m_sumTokenMatchDistance)
    return m_sumTokenMatchDistance > s.m_sumTokenMatchDistance;

  return false;
}

bool KeywordMatcher::ScoreT::operator==(KeywordMatcher::ScoreT const & s) const
{
  return m_sumTokenMatchDistance == s.m_sumTokenMatchDistance
      && m_nameTokensMatched == s.m_nameTokensMatched
      && m_numQueryTokensAndPrefixMatched == s.m_numQueryTokensAndPrefixMatched
      && m_bFullQueryMatched == s.m_bFullQueryMatched
      && m_bPrefixMatched == s.m_bPrefixMatched;
}

bool KeywordMatcher::ScoreT::LessInTokensLength(ScoreT const & s) const
{
  if (m_bFullQueryMatched)
  {
    ASSERT(s.m_bFullQueryMatched, ());
    return m_nameTokensLength > s.m_nameTokensLength;
  }
  return false;
}

string DebugPrint(KeywordMatcher::ScoreT const & score)
{
  ostringstream out;
  out << "KeywordMatcher::ScoreT(";
  out << "FQM=" << score.m_bFullQueryMatched;
  out << ",nQTM=" << static_cast<int>(score.m_numQueryTokensAndPrefixMatched);
  out << ",PM=" << score.m_bPrefixMatched;
  out << ",NTM=";
  for (int i = MAX_TOKENS-1; i >= 0; --i)
    out << ((score.m_nameTokensMatched >> i) & 1);
  out << ",STMD=" << score.m_sumTokenMatchDistance;
  out << ")";
  return out.str();
}
}  // namespace search
