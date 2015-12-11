#include "search/v2/house_numbers_matcher.hpp"

#include "std/algorithm.hpp"
#include "std/iterator.hpp"

#include "base/logging.hpp"

using namespace strings;

namespace search
{
namespace v2
{
namespace
{
HouseNumberTokenizer::CharClass GetCharClass(UniChar c)
{
  static UniString const kSeps = MakeUniString("\"\\/(),. \t№#-");
  if (c >= '0' && c <= '9')
    return HouseNumberTokenizer::CharClass::Digit;
  if (find(kSeps.begin(), kSeps.end(), c) != kSeps.end())
    return HouseNumberTokenizer::CharClass::Separator;
  return HouseNumberTokenizer::CharClass::Other;
}

bool IsShortWord(HouseNumberTokenizer::Token const & t)
{
  return t.m_klass == HouseNumberTokenizer::CharClass::Other && t.m_token.size() <= 3;
}

bool IsNumber(HouseNumberTokenizer::Token const & t)
{
  return t.m_klass == HouseNumberTokenizer::CharClass::Digit;
}

bool IsNumberOrShortWord(HouseNumberTokenizer::Token const & t)
{
  return IsNumber(t) || IsShortWord(t);
}

// Returns number of tokens starting at position |i|,
// where the first token is some way of writing of "корпус", or
// "building",  second token is a number or a letter, and (possibly)
// third token which can be a letter when second token is a
// number.
size_t GetNumTokensForBuildingPart(vector<HouseNumberTokenizer::Token> const & ts, size_t i)
{
  // TODO (@y, @m, @vng): move these constans out.
  static UniString kSynonyms[] = {MakeUniString("building"), MakeUniString("unit"),
                                  MakeUniString("block"),    MakeUniString("корпус"),
                                  MakeUniString("литер"),    MakeUniString("строение"),
                                  MakeUniString("блок")};

  if (i >= ts.size())
    return 0;

  auto const & token = ts[i];
  if (token.m_klass != HouseNumberTokenizer::CharClass::Other)
    return 0;

  bool prefix = false;
  for (UniString const & synonym : kSynonyms)
  {
    if (StartsWith(synonym, token.m_token))
    {
      prefix = true;
      break;
    }
  }
  if (!prefix)
    return 0;

  // No sense in single "корпус" or "литер".
  if (i + 1 >= ts.size() || !IsNumberOrShortWord(ts[i + 1]))
    return 0;

  // Consume next token, either number or short word.
  size_t j = i + 2;

  // Consume one more number of short word, if possible.
  if (j < ts.size() && IsNumberOrShortWord(ts[j]) && ts[j].m_klass != ts[j - 1].m_klass)
    ++j;

  return j - i;
}

void MergeTokens(vector<HouseNumberTokenizer::Token> const & ts, vector<UniString> & rs)
{
  size_t i = 0;
  while (i < ts.size())
  {
    switch (ts[i].m_klass)
    {
    case HouseNumberTokenizer::CharClass::Digit:
    {
      UniString token = ts[i].m_token;
      ++i;
      // Process cases like "123 б" or "9PQ".
      if (i < ts.size() && IsShortWord(ts[i]) && GetNumTokensForBuildingPart(ts, i) == 0)
      {
        token.append(ts[i].m_token.begin(), ts[i].m_token.end());
        ++i;
      }
      rs.push_back(move(token));
      break;
    }
    case HouseNumberTokenizer::CharClass::Separator:
    {
      ASSERT(false, ("Seps can't be merged."));
      ++i;
      break;
    }
    case HouseNumberTokenizer::CharClass::Other:
    {
      if (size_t numTokens = GetNumTokensForBuildingPart(ts, i))
      {
        UniString token = MakeUniString("b.");
        ++i;
        for (size_t j = 1; j < numTokens; ++j, ++i)
          token.append(ts[i].m_token.begin(), ts[i].m_token.end());
        rs.push_back(move(token));
        break;
      }

      rs.push_back(ts[i].m_token);
      ++i;
      break;
    }
    }
  }
}
}  // namespace

// static
void HouseNumberTokenizer::Tokenize(UniString const & s, vector<Token> & ts)
{
  size_t i = 0;
  while (i < s.size())
  {
    CharClass klass = GetCharClass(s[i]);

    size_t j = i;
    while (j < s.size() && GetCharClass(s[j]) == klass)
      ++j;

    if (klass != CharClass::Separator)
    {
      UniString token(s.begin() + i, s.begin() + j);
      ts.emplace_back(move(token), klass);
    }

    i = j;
  }
}

void NormalizeHouseNumber(string const & s, vector<string> & ts)
{
  vector<HouseNumberTokenizer::Token> tokens;
  HouseNumberTokenizer::Tokenize(MakeLowerCase(MakeUniString(s)), tokens);

  vector<UniString> mergedTokens;
  MergeTokens(tokens, mergedTokens);

  transform(mergedTokens.begin(), mergedTokens.end(), back_inserter(ts), &ToUtf8);
}

bool HouseNumbersMatch(string const & houseNumber, string const & query)
{
  if (houseNumber == query)
    return true;

  vector<string> queryTokens;
  NormalizeHouseNumber(query, queryTokens);

  if (!queryTokens.empty())
    sort(queryTokens.begin() + 1, queryTokens.end());

  return HouseNumbersMatch(houseNumber, queryTokens);
}

bool HouseNumbersMatch(string const & houseNumber, vector<string> const & queryTokens)
{
  vector<string> houseNumberTokens;
  NormalizeHouseNumber(houseNumber, houseNumberTokens);

  if (houseNumberTokens.empty() || queryTokens.empty())
    return false;

  // Check first tokens (hope, house numbers).
  if (houseNumberTokens.front() != queryTokens.front())
    return false;

  sort(houseNumberTokens.begin() + 1, houseNumberTokens.end());

  size_t i = 1, j = 1;
  while (i != houseNumberTokens.size() && j != queryTokens.size())
  {
    while (i != houseNumberTokens.size() && houseNumberTokens[i] < queryTokens[j])
      ++i;
    if (i == houseNumberTokens.size() || houseNumberTokens[i] != queryTokens[j])
      return false;
    ++i;
    ++j;
  }
  return j == queryTokens.size();
}
}  // namespace v2
}  // namespace search
