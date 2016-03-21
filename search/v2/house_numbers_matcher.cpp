#include "search/v2/house_numbers_matcher.hpp"

#include "std/algorithm.hpp"
#include "std/iterator.hpp"
#include "std/limits.hpp"
#include "std/sstream.hpp"

#include "base/logging.hpp"

using namespace strings;

namespace search
{
namespace v2
{
namespace
{
size_t constexpr kInvalidNum = numeric_limits<size_t>::max();

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

size_t GetNumTokensForBuildingPart(vector<HouseNumberTokenizer::Token> const & ts, size_t i,
                                   vector<size_t> & memory);

size_t GetNumTokensForBuildingPartImpl(vector<HouseNumberTokenizer::Token> const & ts, size_t i,
                                       vector<size_t> & memory)
{
  ASSERT_LESS(i, ts.size(), ());

  // TODO (@y, @m, @vng): move these constans out.
  static UniString kSynonyms[] = {MakeUniString("building"), MakeUniString("unit"),
                                  MakeUniString("block"),    MakeUniString("корпус"),
                                  MakeUniString("литер"),    MakeUniString("строение"),
                                  MakeUniString("блок")};

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
  if (j < ts.size() && IsNumberOrShortWord(ts[j]) && ts[j].m_klass != ts[j - 1].m_klass &&
      GetNumTokensForBuildingPart(ts, j, memory) == 0)
  {
    ++j;
  }

  return j - i;
}

// Returns number of tokens starting at position |i|, where the first
// token is some way of writing of "корпус", or "building", second
// token is a number or a letter, and (possibly) third token which can
// be a letter when second token is a number. |memory| is used here to
// store results of previous calls and prevents degradation to
// non-linear time.
//
// TODO (@y, @m): the parser is quite complex now. Consider to just
// throw out all prefixes of "building" or "литер" and sort rest
// tokens. Number of false positives will be higher but the parser
// will be more robust, simple and faster.
size_t GetNumTokensForBuildingPart(vector<HouseNumberTokenizer::Token> const & ts, size_t i,
                                   vector<size_t> & memory)
{
  if (i >= ts.size())
    return 0;
  if (memory[i] == kInvalidNum)
    memory[i] = GetNumTokensForBuildingPartImpl(ts, i, memory);
  return memory[i];
}

void MergeTokens(vector<HouseNumberTokenizer::Token> const & ts, vector<UniString> & rs)
{
  vector<size_t> memory(ts.size(), kInvalidNum);

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
      if (i < ts.size() && IsShortWord(ts[i]) && GetNumTokensForBuildingPart(ts, i, memory) == 0)
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
      if (size_t numTokens = GetNumTokensForBuildingPart(ts, i, memory))
      {
        UniString token;
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

void NormalizeHouseNumber(strings::UniString const & s, vector<strings::UniString> & ts)
{
  vector<HouseNumberTokenizer::Token> tokens;
  HouseNumberTokenizer::Tokenize(MakeLowerCase(s), tokens);
  MergeTokens(tokens, ts);

  if (!ts.empty())
    sort(ts.begin() + 1, ts.end());
}

bool HouseNumbersMatch(strings::UniString const & houseNumber, strings::UniString const & query)
{
  if (houseNumber == query)
    return true;

  vector<strings::UniString> queryTokens;
  NormalizeHouseNumber(query, queryTokens);

  return HouseNumbersMatch(houseNumber, queryTokens);
}

bool HouseNumbersMatch(strings::UniString const & houseNumber, vector<strings::UniString> const & queryTokens)
{
  if (houseNumber.empty() || queryTokens.empty())
    return false;
  if (queryTokens[0][0] != houseNumber[0])
    return false;

  vector<strings::UniString> houseNumberTokens;
  NormalizeHouseNumber(houseNumber, houseNumberTokens);

  if (houseNumberTokens.empty())
    return false;

  // Check first tokens (hope, house numbers).
  if (houseNumberTokens.front() != queryTokens.front())
    return false;

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

string DebugPrint(HouseNumberTokenizer::CharClass charClass)
{
  switch (charClass)
  {
  case HouseNumberTokenizer::CharClass::Separator: return "Separator";
  case HouseNumberTokenizer::CharClass::Digit: return "Digit";
  case HouseNumberTokenizer::CharClass::Other: return "Other";
  }
  return "Unknown";
}

string DebugPrint(HouseNumberTokenizer::Token const & token)
{
  ostringstream os;
  os << "Token [" << DebugPrint(token.m_token) << ", " << DebugPrint(token.m_klass) << "]";
  return os.str();
}
}  // namespace v2
}  // namespace search
