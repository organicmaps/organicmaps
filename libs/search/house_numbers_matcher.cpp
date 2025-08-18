#include "search/house_numbers_matcher.hpp"

#include "indexer/string_set.hpp"

#include <algorithm>
#include <iterator>
#include <limits>
#include <sstream>

#include <boost/iterator/transform_iterator.hpp>

using boost::make_transform_iterator;

namespace search
{
namespace house_numbers
{
using namespace std;
using namespace strings;

namespace
{
// Common strings in house numbers.
// To get this list, just run:
//
// ./clusterize-tag-values.lisp house-number-strings path-to-taginfo-db.db > strings.txt
// cat strings.txt |
// awk '{ if ($1 >= 100 && length($3) != 0) { printf("\"%s\",\n", $3) } }' |
// sort | uniq
//
// *NOTE* there is a list of exceptions at the end.

/// @todo By VNG: This list looks hillarious :) Definitely should set some lower bound number
/// to filter very exotic entries in addr:housenumber.

// Removed street keywords for now and ALL one-letter strings. It is sensitive for search speed, because:
// LooksLikeHouseNumber -> MatchBuildingsWithStreets -> *heavy* StreetVicinityLoader::GetStreet
// "av", "avenida",
// "ca", "cal",  "calle",  "carrera", "court",
// "da", "de", "di".
// "ga",
// "ł", "la",
// "ne",
// "pa", "par", "park", "plaza",
// "rd", "ro", "road",
// "so", "south", "st", "street",
// "vi",
// "way", "we", "west",

char const * g_strings[] = {
    "aa", "ab", "abc", "ac", "ad", "ae", "af", "ag", "ah", "ai", "aj", "ak", "al", "am", "an", "ao", "ap", "aq", "ar",
    "are", "as", "at", "au", "aw", "ax", "ay", "az", "azm", "ba", "bab", "bah", "bak", "bb", "bc", "bd", "be", "bedr",
    "ben", "bf", "bg", "bh", "bij", "bis", "bk", "bl", "bldg", "blk", "bloc", "block", "bloco", "blok", "bm", "bmn",
    "bn", "bo", "boe", "bol", "bor", "bov", "box", "bp", "br", "bra", "brc", "bs", "bsa", "bu", "building", "bv", "bwn",
    "bx", "by", "cab", "cat", "cbi", "cbu", "cc", "ccz", "cd", "ce", "centre", "cfn", "cgc", "cjg", "cl", "club",
    "cottage", "cottages", "cso", "cum", "db", "dd", "df", "dia", "dvu", "ec", "ee", "eh", "em", "en", "esm", "ev",
    "fdo", "fer", "ff", "flat", "flats", "floor", "gar", "gara", "gas", "gb", "gg", "gr", "grg", "ha", "haus", "hh",
    "hl", "ho", "house", "hr", "hs", "hv", "ii", "iii", "int", "iv", "ix", "jab", "jf", "jj", "jms", "jtg", "ka", "kab",
    "kk", "kmb", "kmk", "knn", "koy", "kp", "kra", "ksn", "kud", "ldo", "ll", "local", "loja", "lot", "lote", "lsb",
    "lt", "mac", "mad", "mah", "mak", "mat", "mb", "mbb", "mbn", "mch", "mei", "mks", "mm", "mny", "mo", "mok", "mor",
    "msb", "mtj", "mtk", "mvd", "na", "ncc", "nij", "nn", "no", "nr", "nst", "nu", "nut", "of", "ofof", "old", "one",
    "oo", "opl", "pa", "pap", "pav", "pb", "pch", "pg", "ph", "phd", "pkf", "plot", "po", "pos", "pp", "pr", "pra",
    "pya", "qq", "quater", "ra", "rbo", "rear", "reisach", "rk", "rm", "rosso", "rs", "rw", "sab", "sal", "sav", "sb",
    "sba", "sbb", "sbl", "sbn", "sbx", "sc", "sch", "sco", "seb", "sep", "sf", "sgr", "sir", "sj", "sl", "sm", "sn",
    "snc", "som", "sp", "spi", "spn", "ss", "sta", "stc", "std", "stiege", "suite", "sur", "tam", "ter", "terrace",
    "tf", "th", "the", "tl", "to", "torre", "tr", "traf", "trd", "ts", "tt", "tu", "uhm", "unit", "utc", "vii", "wa",
    "wf", "wink", "wrh", "ws", "wsb", "xx", "za", "zh", "zona", "zu", "zw", "א", "ב", "ג", "α", "бб", "бл", "вл", "вх",
    "лит", "разр", "стр", "тп", "уч", "участок", "ა", "丁目", "之", "号", "號",

    // List of exceptions
    "владение"};

// Common strings in house numbers.
// To get this list, just run:
//
// ./clusterize-tag-values.lisp house-number path-to-taginfo-db.db > numbers.txt
// tail -n +2 numbers.txt  | head -78 | sed 's/^.*) \(.*\) \[.*$/"\1"/g;s/[ -/]//g;s/$/,/' |
// sort | uniq
vector<string> const g_patterns = {"BL", "BLN", "BLNSL", "BN", "BNL", "BNSL", "L", "LL", "LN", "LNL", "LNLN", "LNN",
                                   "N", "NBL", "NBLN", "NBN", "NBNBN", "NBNL", "NL", "NLBN", "NLL", "NLLN", "NLN",
                                   "NLNL", "NLS", "NLSN", "NN", "NNBN", "NNL", "NNLN", "NNN", "NNS", "NS", "NSN", "NSS",
                                   "S", "SL", "SLL", "SLN", "SN", "SNBNSS", "SNL", "SNN", "SS", "SSN", "SSS", "SSSS",

                                   // List of exceptions
                                   "NNBNL"};

// List of patterns which look like house numbers more than other patterns. Constructed by hand.
vector<string> const g_patternsStrict = {"N", "NBN", "NBL", "NL"};

// List of common synonyms for building parts. Constructed by hand.
char const * g_buildingPartSynonyms[] = {"building", "bldg", "bld",   "bl",  "unit",     "block", "blk",  "корпус",
                                         "корп",     "кор",  "литер", "лит", "строение", "стр",   "блок", "бл"};

// List of common stop words for buildings. Constructed by hand.
UniString const g_stopWords[] = {MakeUniString("дом"), MakeUniString("house"), MakeUniString("д")};

bool IsStopWord(UniString const & s, bool isPrefix)
{
  for (auto const & p : g_stopWords)
    if ((isPrefix && StartsWith(p, s)) || (!isPrefix && p == s))
      return true;
  return false;
}

class BuildingPartSynonymsMatcher
{
public:
  using Synonyms = StringSet<UniChar, 4>;

  BuildingPartSynonymsMatcher()
  {
    for (auto const & s : g_buildingPartSynonyms)
    {
      UniString const us = MakeUniString(s);
      m_synonyms.Add(us.begin(), us.end());
    }
  }

  // Returns true if |s| looks like a building synonym.
  inline bool Has(UniString const & s) const { return m_synonyms.Has(s.begin(), s.end()) == Synonyms::Status::Full; }

private:
  Synonyms m_synonyms;
};

class StringsMatcher
{
public:
  using Strings = StringSet<UniChar, 8>;

  StringsMatcher()
  {
    for (auto const & s : g_strings)
    {
      UniString const us = MakeUniString(s);
      m_strings.Add(us.begin(), us.end());
    }

    for (auto const & s : g_buildingPartSynonyms)
    {
      UniString const us = MakeUniString(s);
      m_strings.Add(us.begin(), us.end());
    }
  }

  // Returns true when |s| may be a full substring of a house number,
  // or a prefix of some valid substring of a house number, when
  // |isPrefix| is true.
  bool Has(UniString const & s, bool isPrefix) const
  {
    auto const status = m_strings.Has(s.begin(), s.end());
    switch (status)
    {
    case Strings::Status::Absent: return false;
    case Strings::Status::Prefix: return isPrefix;
    case Strings::Status::Full: return true;
    }
    UNREACHABLE();
  }

private:
  Strings m_strings;
};

class HouseNumberClassifier
{
public:
  using Patterns = StringSet<Token::Type, 4>;

  HouseNumberClassifier(vector<string> const & patterns = g_patterns)
  {
    for (auto const & p : patterns)
      m_patterns.Add(make_transform_iterator(p.begin(), &CharToType), make_transform_iterator(p.end(), &CharToType));
  }

  // Returns true when the string |s| looks like a valid house number,
  // (or a prefix of some valid house number, when |isPrefix| is
  // true).
  bool LooksGood(UniString const & s, bool isPrefix) const
  {
    TokensT parse;
    Tokenize(s, isPrefix, parse);

    size_t i = 0;
    for (size_t j = 0; j != parse.size(); ++j)
    {
      auto const & token = parse[j];
      auto const type = token.m_type;
      switch (type)
      {
      case Token::TYPE_SEPARATOR: break;
      case Token::TYPE_GROUP_SEPARATOR: break;
      case Token::TYPE_HYPHEN: break;
      case Token::TYPE_SLASH: break;
      case Token::TYPE_STRING:
      {
        if (IsStopWord(token.m_value, token.m_prefix))
          break;
        if (!m_matcher.Has(token.m_value, token.m_prefix))
          return false;
        [[fallthrough]];
      }
      case Token::TYPE_LETTER:
      {
        if (j == 0 && IsStopWord(token.m_value, token.m_prefix))
          break;
        [[fallthrough]];
      }
      case Token::TYPE_NUMBER:
      case Token::TYPE_BUILDING_PART:
      case Token::TYPE_BUILDING_PART_OR_LETTER:
        parse[i] = std::move(parse[j]);
        ASSERT(!parse[i].m_value.empty(), ());
        ++i;
      }
    }
    parse.resize(i);

    auto const status = m_patterns.Has(make_transform_iterator(parse.begin(), &TokenToType),
                                       make_transform_iterator(parse.end(), &TokenToType));
    switch (status)
    {
    case Patterns::Status::Absent: return false;
    case Patterns::Status::Prefix: return true;
    case Patterns::Status::Full: return true;
    }
    UNREACHABLE();
  }

private:
  static Token::Type CharToType(char c)
  {
    switch (c)
    {
    case 'N': return Token::TYPE_NUMBER;
    case 'S': return Token::TYPE_STRING;
    case 'B': return Token::TYPE_BUILDING_PART;
    case 'L': return Token::TYPE_LETTER;
    case 'U': return Token::TYPE_BUILDING_PART_OR_LETTER;
    default: CHECK(false, ("Unexpected character:", c)); return Token::TYPE_SEPARATOR;
    }
    UNREACHABLE();
  }

  static Token::Type TokenToType(Token const & token) { return token.m_type; }

  StringsMatcher m_matcher;
  Patterns m_patterns;
};

Token::Type GetCharType(UniChar c)
{
  static UniString const kSeps = MakeUniString(" \t\"\\().#~");
  static UniString const kGroupSeps = MakeUniString(",|;+");

  if (IsASCIIDigit(c))
    return Token::TYPE_NUMBER;
  if (find(kSeps.begin(), kSeps.end(), c) != kSeps.end())
    return Token::TYPE_SEPARATOR;
  if (find(kGroupSeps.begin(), kGroupSeps.end(), c) != kGroupSeps.end())
    return Token::TYPE_GROUP_SEPARATOR;
  if (c == '-')
    return Token::TYPE_HYPHEN;
  if (c == '/')
    return Token::TYPE_SLASH;
  return Token::TYPE_STRING;
}

bool IsLiteralType(Token::Type type)
{
  return type == Token::TYPE_STRING || type == Token::TYPE_LETTER || type == Token::TYPE_BUILDING_PART_OR_LETTER;
}

// Leaves only numbers and letters, removes all trailing prefix
// tokens. Then, does following:
//
// * when there is at least one number, drops all tokens until the
//   number and sorts the rest
// * when there are no numbers at all, sorts tokens
void SimplifyParse(TokensT & tokens)
{
  if (!tokens.empty() && tokens.back().m_prefix)
    tokens.pop_back();

  size_t i = 0;
  size_t j = 0;
  while (j != tokens.size() && tokens[j].m_type != Token::TYPE_NUMBER)
    ++j;
  for (; j != tokens.size(); ++j)
  {
    auto const type = tokens[j].m_type;
    if (type == Token::TYPE_NUMBER || type == Token::TYPE_LETTER)
      tokens[i++] = tokens[j];
  }

  if (i != 0)
  {
    tokens.resize(i);
    sort(tokens.begin() + 1, tokens.end());
  }
  else
  {
    sort(tokens.begin(), tokens.end());
  }
}

// Returns true when a sequence denoted by [b2, e2) is a subsequence
// of [b1, e1).
template <typename T1, typename T2>
bool IsSubsequence(T1 b1, T1 e1, T2 b2, T2 e2)
{
  for (; b2 != e2; ++b1, ++b2)
  {
    while (b1 != e1 && *b1 < *b2)
      ++b1;
    if (b1 == e1 || *b1 != *b2)
      return false;
  }
  return true;
}

bool IsBuildingPartSynonym(UniString const & s)
{
  static BuildingPartSynonymsMatcher const kMatcher;
  return kMatcher.Has(s);
}

bool IsShortBuildingSynonym(UniString const & t)
{
  static UniString const kSynonyms[] = {MakeUniString("к"), MakeUniString("с")};
  for (auto const & s : kSynonyms)
    if (t == s)
      return true;
  return false;
}

template <typename Fn>
void ForEachGroup(TokensT const & ts, Fn && fn)
{
  size_t i = 0;
  while (i < ts.size())
  {
    while (i < ts.size() && ts[i].m_type == Token::TYPE_GROUP_SEPARATOR)
      ++i;

    size_t j = i;
    while (j < ts.size() && ts[j].m_type != Token::TYPE_GROUP_SEPARATOR)
      ++j;

    if (i != j)
      fn(i, j);

    i = j;
  }
}

template <typename Fn>
void TransformString(UniString && token, Fn && fn)
{
  static UniString const kLiter = MakeUniString("лит");

  size_t const size = token.size();

  if (IsBuildingPartSynonym(token))
  {
    fn(std::move(token), Token::TYPE_BUILDING_PART);
  }
  else if (size == 4 && StartsWith(token, kLiter))
  {
    fn(UniString(token.begin(), token.begin() + 3), Token::TYPE_BUILDING_PART);
    fn(UniString(token.begin() + 3, token.end()), Token::TYPE_LETTER);
  }
  else if (size == 2)
  {
    UniString firstLetter(token.begin(), token.begin() + 1);
    if (IsShortBuildingSynonym(firstLetter))
    {
      fn(std::move(firstLetter), Token::TYPE_BUILDING_PART);
      fn(UniString(token.begin() + 1, token.end()), Token::TYPE_LETTER);
    }
    else
    {
      fn(std::move(token), Token::TYPE_STRING);
    }
  }
  else if (size == 1)
  {
    if (IsShortBuildingSynonym(token))
      fn(std::move(token), Token::TYPE_BUILDING_PART_OR_LETTER);
    else
      fn(std::move(token), Token::TYPE_LETTER);
  }
  else
  {
    fn(std::move(token), Token::TYPE_STRING);
  }
}
}  // namespace

uint64_t ToUInt(UniString const & s)
{
  uint64_t res = 0;
  uint64_t pow = 1;

  int i = int(s.size()) - 1;
  ASSERT(i >= 0 && i < std::numeric_limits<uint64_t>::digits10, (i));
  for (; i >= 0; --i)
  {
    ASSERT(IsASCIIDigit(s[i]), (s[i]));

    res += (s[i] - '0') * pow;
    pow *= 10;
  }
  return res;
}

void Tokenize(UniString s, bool isPrefix, TokensT & ts)
{
  MakeLowerCaseInplace(s);
  auto addToken = [&ts](UniString && value, Token::Type type) { ts.emplace_back(std::move(value), type); };

  size_t i = 0;
  while (i < s.size())
  {
    Token::Type const type = GetCharType(s[i]);

    size_t j = i + 1;
    while (j < s.size() && GetCharType(s[j]) == type)
      ++j;

    if (type != Token::TYPE_SEPARATOR)
    {
      UniString token(s.begin() + i, s.begin() + j);
      if (type == Token::TYPE_STRING)
      {
        if (j != s.size() || !isPrefix)
        {
          TransformString(std::move(token), addToken);
        }
        else if (i + 1 == j)
        {
          ts.emplace_back(std::move(token), Token::TYPE_LETTER);
        }
        else
        {
          ts.emplace_back(std::move(token), Token::TYPE_STRING);
          ts.back().m_prefix = true;
        }
      }
      else
      {
        addToken(std::move(token), type);
      }
    }

    i = j;
  }

  // Quite hacky loop from ts.size() - 1 towards 0.
  for (size_t i = ts.size() - 1; i < ts.size(); --i)
  {
    if (ts[i].m_type != Token::TYPE_BUILDING_PART_OR_LETTER)
      continue;
    if (i + 1 == ts.size() || ts[i + 1].m_type == Token::TYPE_BUILDING_PART)
      ts[i].m_type = Token::TYPE_LETTER;
    else if (ts[i + 1].m_type == Token::TYPE_NUMBER)
      ts[i].m_type = Token::TYPE_BUILDING_PART;
  }
}

void ParseHouseNumber(UniString const & s, vector<TokensT> & parses)
{
  TokensT tokens;
  Tokenize(s, false /* isPrefix */, tokens);

  bool numbersSequence = true;
  ForEachGroup(tokens, [&tokens, &numbersSequence](size_t i, size_t j)
  {
    switch (j - i)
    {
    case 0: break;
    case 1: numbersSequence = numbersSequence && tokens[i].m_type == Token::TYPE_NUMBER; break;
    case 2:
      numbersSequence =
          numbersSequence && tokens[i].m_type == Token::TYPE_NUMBER && IsLiteralType(tokens[i + 1].m_type);
      break;
    default: numbersSequence = false; break;
    }
  });

  size_t const oldSize = parses.size();
  if (numbersSequence)
  {
    ForEachGroup(tokens, [&tokens, &parses](size_t i, size_t j)
    {
      parses.emplace_back();
      auto & parse = parses.back();
      for (size_t k = i; k < j; ++k)
        parse.emplace_back(std::move(tokens[k]));
    });
  }
  else
  {
    parses.emplace_back(std::move(tokens));
  }

  for (size_t i = oldSize; i < parses.size(); ++i)
    SimplifyParse(parses[i]);
}

void ParseQuery(UniString const & query, bool queryIsPrefix, TokensT & parse)
{
  Tokenize(query, queryIsPrefix, parse);
  SimplifyParse(parse);
}

bool HouseNumbersMatch(UniString const & houseNumber, TokensT const & queryParse)
{
  if (houseNumber.empty() || queryParse.empty())
    return false;

  // Fast pre-check, helps to early exit without complex house number parsing.
  if (IsASCIIDigit(houseNumber[0]) && IsASCIIDigit(queryParse[0].m_value[0]) &&
      houseNumber[0] != queryParse[0].m_value[0])
  {
    return false;
  }

  vector<TokensT> houseNumberParses;
  ParseHouseNumber(houseNumber, houseNumberParses);

  for (auto & parse : houseNumberParses)
  {
    if (parse.empty())
      continue;
    if (parse[0] == queryParse[0] &&
        (IsSubsequence(parse.begin() + 1, parse.end(), queryParse.begin() + 1, queryParse.end()) ||
         IsSubsequence(queryParse.begin() + 1, queryParse.end(), parse.begin() + 1, parse.end())))
    {
      return true;
    }
  }
  return false;
}

bool HouseNumbersMatchConscription(UniString const & houseNumber, TokensT const & queryParse)
{
  auto const beg = houseNumber.begin();
  auto const end = houseNumber.end();
  auto i = std::find(beg, end, '/');
  if (i != end)
  {
    // Conscription number / street number.
    return HouseNumbersMatch(UniString(beg, i), queryParse) || HouseNumbersMatch(UniString(i + 1, end), queryParse);
  }
  return HouseNumbersMatch(houseNumber, queryParse);
}

bool HouseNumbersMatchRange(std::string_view const & hnRange, TokensT const & queryParse,
                            feature::InterpolType interpol)
{
  ASSERT(interpol != feature::InterpolType::None, ());

  if (queryParse[0].m_type != Token::TYPE_NUMBER)
    return false;

  uint64_t const val = ToUInt(queryParse[0].m_value);
  bool const isEven = (val % 2 == 0);
  if (interpol == feature::InterpolType::Odd && isEven)
    return false;
  if (interpol == feature::InterpolType::Even && !isEven)
    return false;

  // Generator makes valid normalized values.
  size_t const i = hnRange.find(':');
  if (i == std::string_view::npos)
  {
    ASSERT(false, (hnRange));
    return false;
  }

  uint64_t left, right;
  if (!strings::to_uint(hnRange.substr(0, i), left) || !strings::to_uint(hnRange.substr(i + 1), right))
  {
    ASSERT(false, (hnRange));
    return false;
  }

  return left < val && val < right;
}

bool LooksLikeHouseNumber(UniString const & s, bool isPrefix)
{
  static HouseNumberClassifier const classifier;
  return classifier.LooksGood(s, isPrefix);
}

bool LooksLikeHouseNumber(string const & s, bool isPrefix)
{
  return LooksLikeHouseNumber(MakeUniString(s), isPrefix);
}

bool LooksLikeHouseNumberStrict(UniString const & s)
{
  static HouseNumberClassifier const classifier(g_patternsStrict);
  return classifier.LooksGood(s, false /* isPrefix */);
}

bool LooksLikeHouseNumberStrict(string const & s)
{
  return LooksLikeHouseNumberStrict(MakeUniString(s));
}

string DebugPrint(Token::Type type)
{
  switch (type)
  {
  case Token::TYPE_NUMBER: return "Number";
  case Token::TYPE_SEPARATOR: return "Separator";
  case Token::TYPE_GROUP_SEPARATOR: return "GroupSeparator";
  case Token::TYPE_HYPHEN: return "Hyphen";
  case Token::TYPE_SLASH: return "Slash";
  case Token::TYPE_STRING: return "String";
  case Token::TYPE_BUILDING_PART: return "BuildingPart";
  case Token::TYPE_LETTER: return "Letter";
  case Token::TYPE_BUILDING_PART_OR_LETTER: return "BuildingPartOrLetter";
  }
  return "Unknown";
}

string DebugPrint(Token const & token)
{
  ostringstream os;
  os << "Token [" << DebugPrint(token.m_value) << ", " << DebugPrint(token.m_type) << "]";
  return os.str();
}
}  // namespace house_numbers
}  // namespace search
