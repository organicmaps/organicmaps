#include "search/ranking_utils.hpp"

#include "base/dfa_helpers.hpp"

#include "std/transform_iterator.hpp"

#include <algorithm>
#include <sstream>

using namespace strings;

namespace search
{
namespace
{
UniString AsciiToUniString(char const * s) { return UniString(s, s + strlen(s)); }
}  // namespace

string DebugPrint(ErrorsMade const & errorsMade)
{
  ostringstream os;
  os << "ErrorsMade [ ";
  if (errorsMade.IsValid())
    os << errorsMade.m_errorsMade;
  else
    os << "invalid";
  os << " ]";
  return os.str();
}

namespace impl
{
bool FullMatch(QueryParams::Token const & token, UniString const & text)
{
  if (token.m_original == text)
    return true;
  auto const & synonyms = token.m_synonyms;
  return find(synonyms.begin(), synonyms.end(), text) != synonyms.end();
}

bool PrefixMatch(QueryParams::Token const & token, UniString const & text)
{
  if (StartsWith(text, token.m_original))
    return true;

  for (auto const & synonym : token.m_synonyms)
  {
    if (StartsWith(text, synonym))
      return true;
  }
  return false;
}

ErrorsMade GetMinErrorsMade(std::vector<strings::UniString> const & tokens,
                            strings::UniString const & text)
{
  auto const dfa = BuildLevenshteinDFA(text);

  ErrorsMade errorsMade;

  for (auto const & token : tokens)
  {
    auto it = dfa.Begin();
    strings::DFAMove(it, token.begin(), token.end());
    if (it.Accepts())
      errorsMade = ErrorsMade::Min(errorsMade, ErrorsMade(it.ErrorsMade()));
  }

  return errorsMade;
}
}  // namespace impl

bool IsStopWord(UniString const & s)
{
  /// @todo Get all common used stop words and factor out this array into
  /// search_string_utils.cpp module for example.
  static char const * arr[] = {"a", "de", "da", "la"};

  static std::set<UniString> const kStopWords(
      make_transform_iterator(arr, &AsciiToUniString),
      make_transform_iterator(arr + ARRAY_SIZE(arr), &AsciiToUniString));

  return kStopWords.count(s) > 0;
}

void PrepareStringForMatching(std::string const & name, std::vector<strings::UniString> & tokens)
{
  auto filter = [&tokens](strings::UniString const & token)
  {
    if (!IsStopWord(token))
      tokens.push_back(token);
  };
  SplitUniString(NormalizeAndSimplifyString(name), filter, Delimiters());
}

string DebugPrint(NameScore score)
{
  switch (score)
  {
  case NAME_SCORE_ZERO: return "Zero";
  case NAME_SCORE_SUBSTRING: return "Substring";
  case NAME_SCORE_PREFIX: return "Prefix";
  case NAME_SCORE_FULL_MATCH: return "Full Match";
  case NAME_SCORE_COUNT: return "Count";
  }
  return "Unknown";
}
}  // namespace search
