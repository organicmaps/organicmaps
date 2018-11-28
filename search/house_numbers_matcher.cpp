#include "search/house_numbers_matcher.hpp"

#include "indexer/string_set.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/iterator.hpp"
#include "std/limits.hpp"
#include "std/sstream.hpp"
#include "std/transform_iterator.hpp"

using namespace strings;

namespace search
{
namespace house_numbers
{
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
char const * const g_strings[] = {
    "a",      "aa",      "ab",      "abc",  "ac",   "ad",      "ae",      "af",       "ag",
    "ah",     "ai",      "aj",      "ak",   "al",   "am",      "an",      "ao",       "ap",
    "aq",     "ar",      "are",     "as",   "at",   "au",      "av",      "avenida",  "aw",
    "ax",     "ay",      "az",      "azm",  "b",    "ba",      "bab",     "bah",      "bak",
    "bb",     "bc",      "bd",      "be",   "bedr", "ben",     "bf",      "bg",       "bh",
    "bij",    "bis",     "bk",      "bl",   "bldg", "blk",     "bloc",    "block",    "bloco",
    "blok",   "bm",      "bmn",     "bn",   "bo",   "boe",     "bol",     "bor",      "bov",
    "box",    "bp",      "br",      "bra",  "brc",  "bs",      "bsa",     "bu",       "building",
    "bus",    "bv",      "bwn",     "bx",   "by",   "c",       "ca",      "cab",      "cal",
    "calle",  "carrera", "cat",     "cbi",  "cbu",  "cc",      "ccz",     "cd",       "ce",
    "centre", "cfn",     "cgc",     "cjg",  "cl",   "club",    "cottage", "cottages", "court",
    "cso",    "cum",     "d",       "da",   "db",   "dd",      "de",      "df",       "di",
    "dia",    "dvu",     "e",       "ec",   "ee",   "eh",      "em",      "en",       "esm",
    "ev",     "f",       "farm",    "fdo",  "fer",  "ff",      "fixme",   "flat",     "flats",
    "floor",  "g",       "ga",      "gar",  "gara", "gas",     "gb",      "gg",       "gr",
    "grg",    "h",       "ha",      "haus", "hh",   "hl",      "ho",      "house",    "hr",
    "hs",     "hv",      "i",       "ii",   "iii",  "int",     "iv",      "ix",       "j",
    "jab",    "jf",      "jj",      "jms",  "jtg",  "k",       "ka",      "kab",      "kk",
    "km",     "kmb",     "kmk",     "knn",  "koy",  "kp",      "kra",     "ksn",      "kud",
    "l",      "ł",       "la",      "ldo",  "ll",   "local",   "loja",    "lot",      "lote",
    "lsb",    "lt",      "m",       "mac",  "mad",  "mah",     "mak",     "mat",      "mb",
    "mbb",    "mbn",     "mch",     "mei",  "mks",  "mm",      "mny",     "mo",       "mok",
    "monica", "mor",     "morocco", "msb",  "mtj",  "mtk",     "mvd",     "n",        "na",
    "ncc",    "ne",      "nij",     "nn",   "no",   "nr",      "nst",     "nu",       "nut",
    "o",      "of",      "ofof",    "old",  "one",  "oo",      "opl",     "p",        "pa",
    "pap",    "par",     "park",    "pav",  "pb",   "pch",     "pg",      "ph",       "phd",
    "pkf",    "plaza",   "plot",    "po",   "pos",  "pp",      "pr",      "pra",      "pya",
    "q",      "qq",      "quater",  "r",    "ra",   "rbo",     "rd",      "rear",     "reisach",
    "rk",     "rm",      "ro",      "road", "rood", "rosso",   "rs",      "rw",       "s",
    "sab",    "sal",     "sav",     "sb",   "sba",  "sbb",     "sbl",     "sbn",      "sbx",
    "sc",     "sch",     "sco",     "seb",  "sep",  "sf",      "sgr",     "shop",     "sir",
    "sj",     "sl",      "sm",      "sn",   "snc",  "so",      "som",     "south",    "sp",
    "spi",    "spn",     "ss",      "st",   "sta",  "stc",     "std",     "stiege",   "street",
    "suite",  "sur",     "t",       "tam",  "ter",  "terrace", "tf",      "th",       "the",
    "tl",     "to",      "torre",   "tr",   "traf", "trd",     "ts",      "tt",       "tu",
    "u",      "uhm",     "unit",    "utc",  "v",    "vi",      "vii",     "w",        "wa",
    "way",    "we",      "west",    "wf",   "wink", "wrh",     "ws",      "wsb",      "x",
    "xx",     "y",       "z",       "za",   "zh",   "zona",    "zu",      "zw",       "א",
    "ב",      "ג",       "α",       "а",    "б",    "бб",      "бл",      "в",        "вл",
    "вх",     "г",       "д",       "е",    "ж",    "з",       "и",       "к",        "л",
    "лит",    "м",       "магазин", "н",    "о",    "п",       "р",       "разр",     "с",
    "стр",    "т",       "тп",      "у",    "уч",   "участок", "ф",       "ц",        "ა",
    "丁目",   "之",      "号",      "號",

    // List of exceptions
    "владение"
};

// Common strings in house numbers.
// To get this list, just run:
//
// ./clusterize-tag-values.lisp house-number path-to-taginfo-db.db > numbers.txt
// tail -n +2 numbers.txt  | head -78 | sed 's/^.*) \(.*\) \[.*$/"\1"/g;s/[ -/]//g;s/$/,/' |
// sort | uniq
const char * const g_patterns[] = {
    "BL",  "BLN",  "BLNSL", "BN",   "BNL",    "BNSL", "L",   "LL",   "LN",  "LNL",  "LNLN", "LNN",
    "N",   "NBL",  "NBLN",  "NBN",  "NBNBN",  "NBNL", "NL",  "NLBN", "NLL", "NLLN", "NLN",  "NLNL",
    "NLS", "NLSN", "NN",    "NNBN", "NNL",    "NNLN", "NNN", "NNS",  "NS",  "NSN",  "NSS",  "S",
    "SL",  "SLL",  "SLN",   "SN",   "SNBNSS", "SNL",  "SNN", "SS",   "SSN", "SSS",  "SSSS",

    // List of exceptions
    "NNBNL"
};

// List of common synonyms for building parts. Constructed by hand.
const char * const g_buildingPartSynonyms[] = {
    "building", "bldg", "bld",   "bl",  "unit",     "block", "blk",  "корпус",
    "корп",     "кор",  "литер", "лит", "строение", "стр",   "блок", "бл"};

// List of common stop words for buildings. Constructed by hand.
UniString const g_stopWords[] = {MakeUniString("дом"), MakeUniString("house"), MakeUniString("д")};

bool IsStopWord(UniString const & s, bool isPrefix)
{
  for (auto const & p : g_stopWords)
  {
    if ((isPrefix && StartsWith(p, s)) || (!isPrefix && p == s))
      return true;
  }
  return false;
}

class BuildingPartSynonymsMatcher
{
public:
  using TSynonyms = StringSet<UniChar, 4>;

  BuildingPartSynonymsMatcher()
  {
    for (auto const * s : g_buildingPartSynonyms)
    {
      UniString const us = MakeUniString(s);
      m_synonyms.Add(us.begin(), us.end());
    }
  }

  // Returns true if |s| looks like a building synonym.
  inline bool Has(UniString const & s) const
  {
    return m_synonyms.Has(s.begin(), s.end()) == TSynonyms::Status::Full;
  }

private:
  TSynonyms m_synonyms;
};

class StringsMatcher
{
public:
  using TStrings = StringSet<UniChar, 8>;

  StringsMatcher()
  {
    for (auto const * s : g_strings)
    {
      UniString const us = MakeUniString(s);
      m_strings.Add(us.begin(), us.end());
    }

    for (auto const * s : g_buildingPartSynonyms)
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
    case TStrings::Status::Absent: return false;
    case TStrings::Status::Prefix: return isPrefix;
    case TStrings::Status::Full: return true;
    }
    UNREACHABLE();
  }

private:
  TStrings m_strings;
};

class HouseNumberClassifier
{
public:
  using TPatterns = StringSet<Token::Type, 4>;

  HouseNumberClassifier()
  {
    for (auto const * p : g_patterns)
    {
      m_patterns.Add(make_transform_iterator(p, &CharToType),
                     make_transform_iterator(p + strlen(p), &CharToType));
    }
  }

  // Returns true when the string |s| looks like a valid house number,
  // (or a prefix of some valid house number, when |isPrefix| is
  // true).
  bool LooksGood(UniString const & s, bool isPrefix) const
  {
    vector<Token> parse;
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
        // fallthrough
      }
      case Token::TYPE_LETTER:
      {
        if (j == 0 && IsStopWord(token.m_value, token.m_prefix))
          break;
        // fallthrough
      }
      case Token::TYPE_NUMBER:         // fallthrough
      case Token::TYPE_BUILDING_PART:  // fallthrough
      case Token::TYPE_BUILDING_PART_OR_LETTER:
        parse[i] = move(parse[j]);
        ++i;
      }
    }
    parse.resize(i);

    auto const status = m_patterns.Has(make_transform_iterator(parse.begin(), &TokenToType),
                                       make_transform_iterator(parse.end(), &TokenToType));
    switch (status)
    {
    case TPatterns::Status::Absent: return false;
    case TPatterns::Status::Prefix: return true;
    case TPatterns::Status::Full: return true;
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
  TPatterns m_patterns;
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
  return type == Token::TYPE_STRING || type == Token::TYPE_LETTER ||
         type == Token::TYPE_BUILDING_PART_OR_LETTER;
}

// Leaves only numbers and letters, removes all trailing prefix
// tokens. Then, does following:
//
// * when there is at least one number, drops all tokens until the
//   number and sorts the rest
// * when there are no numbers at all, sorts tokens
void SimplifyParse(vector<Token> & tokens)
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
  {
    if (t == s)
      return true;
  }
  return false;
}

template <typename TFn>
void ForEachGroup(vector<Token> const & ts, TFn && fn)
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

template <typename TFn>
void TransformString(UniString && token, TFn && fn)
{
  static UniString const kLiter = MakeUniString("лит");

  size_t const size = token.size();

  if (IsBuildingPartSynonym(token))
  {
    fn(move(token), Token::TYPE_BUILDING_PART);
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
      fn(move(firstLetter), Token::TYPE_BUILDING_PART);
      fn(UniString(token.begin() + 1, token.end()), Token::TYPE_LETTER);
    }
    else
    {
      fn(move(token), Token::TYPE_STRING);
    }
  }
  else if (size == 1)
  {
    if (IsShortBuildingSynonym(token))
      fn(move(token), Token::TYPE_BUILDING_PART_OR_LETTER);
    else
      fn(move(token), Token::TYPE_LETTER);
  }
  else
  {
    fn(move(token), Token::TYPE_STRING);
  }
}
}  // namespace

void Tokenize(UniString s, bool isPrefix, vector<Token> & ts)
{
  MakeLowerCaseInplace(s);
  auto addToken = [&ts](UniString && value, Token::Type type)
  {
    ts.emplace_back(move(value), type);
  };

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
          TransformString(move(token), addToken);
        }
        else if (i + 1 == j)
        {
          ts.emplace_back(move(token), Token::TYPE_LETTER);
        }
        else
        {
          ts.emplace_back(move(token), Token::TYPE_STRING);
          ts.back().m_prefix = true;
        }
      }
      else
      {
        addToken(move(token), type);
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

void ParseHouseNumber(strings::UniString const & s, vector<vector<Token>> & parses)
{
  vector<Token> tokens;
  Tokenize(s, false /* isPrefix */, tokens);

  bool numbersSequence = true;
  ForEachGroup(tokens, [&tokens, &numbersSequence](size_t i, size_t j)
               {
                 switch (j - i)
                 {
                 case 0: break;
                 case 1:
                   numbersSequence = numbersSequence && tokens[i].m_type == Token::TYPE_NUMBER;
                   break;
                 case 2:
                   numbersSequence = numbersSequence && tokens[i].m_type == Token::TYPE_NUMBER &&
                                     IsLiteralType(tokens[i + 1].m_type);
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
                     parse.emplace_back(move(tokens[k]));
                 });
  }
  else
  {
    parses.emplace_back(move(tokens));
  }

  for (size_t i = oldSize; i < parses.size(); ++i)
    SimplifyParse(parses[i]);
}

void ParseQuery(strings::UniString const & query, bool queryIsPrefix, vector<Token> & parse)
{
  Tokenize(query, queryIsPrefix, parse);
  SimplifyParse(parse);
}

bool HouseNumbersMatch(strings::UniString const & houseNumber, strings::UniString const & query,
                       bool queryIsPrefix)
{
  if (houseNumber == query)
    return true;

  vector<Token> queryParse;
  ParseQuery(query, queryIsPrefix, queryParse);

  return HouseNumbersMatch(houseNumber, queryParse);
}

bool HouseNumbersMatch(strings::UniString const & houseNumber, vector<Token> const & queryParse)
{
  if (houseNumber.empty() || queryParse.empty())
    return false;

  // Fast pre-check, helps to early exit without complex house number
  // parsing.
  if (IsASCIIDigit(houseNumber[0]) && IsASCIIDigit(queryParse[0].m_value[0]) &&
      houseNumber[0] != queryParse[0].m_value[0])
  {
    return false;
  }

  vector<vector<Token>> houseNumberParses;
  ParseHouseNumber(houseNumber, houseNumberParses);

  for (auto & parse : houseNumberParses)
  {
    if (parse.empty())
      continue;
    if (parse[0] == queryParse[0] &&
        IsSubsequence(parse.begin() + 1, parse.end(), queryParse.begin() + 1, queryParse.end()))
    {
      return true;
    }
  }
  return false;
}

bool LooksLikeHouseNumber(strings::UniString const & s, bool isPrefix)
{
  static HouseNumberClassifier const classifier;
  return classifier.LooksGood(s, isPrefix);
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
