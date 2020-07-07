#include "search/ranking_utils.hpp"

#include "search/token_slice.hpp"
#include "search/utils.hpp"

#include "base/dfa_helpers.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/feature_data.hpp"

#include <algorithm>
#include <sstream>

#include <boost/iterator/transform_iterator.hpp>

using namespace std;
using namespace strings;

using boost::make_transform_iterator;

namespace search
{
namespace
{
struct TokenInfo
{
  bool m_isCategoryToken = false;
  bool m_inFeatureTypes = false;
};
}  // namespace

// CategoriesInfo ----------------------------------------------------------------------------------
CategoriesInfo::CategoriesInfo(feature::TypesHolder const & holder, TokenSlice const & tokens,
                               Locales const & locales, CategoriesHolder const & categories)
{
  QuerySlice slice(tokens);
  vector<TokenInfo> infos(slice.Size());
  ForEachCategoryType(slice, locales, categories, [&holder, &infos](size_t i, uint32_t t) {
    ASSERT_LESS(i, infos.size(), ());
    auto & info = infos[i];

    info.m_isCategoryToken = true;
    if (holder.Has(t))
      info.m_inFeatureTypes = true;
  });

  // Note that m_inFeatureTypes implies m_isCategoryToken.

  m_pureCategories = all_of(infos.begin(), infos.end(), [](TokenInfo const & info) {
    ASSERT(!info.m_inFeatureTypes || info.m_isCategoryToken, ());
    return info.m_inFeatureTypes != 0;
  });

  m_falseCategories = all_of(infos.begin(), infos.end(), [](TokenInfo const & info) {
    return !info.m_inFeatureTypes && info.m_isCategoryToken;
  });
}

// ErrorsMade --------------------------------------------------------------------------------------
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
ErrorsMade GetErrorsMade(QueryParams::Token const & token, strings::UniString const & text)
{
  if (token.AnyOfSynonyms([&text](strings::UniString const & s) { return text == s; }))
    return ErrorsMade(0);

  auto const dfa = BuildLevenshteinDFA(text);
  auto it = dfa.Begin();
  strings::DFAMove(it, token.GetOriginal().begin(), token.GetOriginal().end());
  if (it.Accepts())
    return ErrorsMade(it.ErrorsMade());

  return {};
}

ErrorsMade GetPrefixErrorsMade(QueryParams::Token const & token, strings::UniString const & text)
{
  if (token.AnyOfSynonyms([&text](strings::UniString const & s) { return StartsWith(text, s); }))
    return ErrorsMade(0);

  auto const dfa = PrefixDFAModifier<LevenshteinDFA>(BuildLevenshteinDFA(text));
  auto it = dfa.Begin();
  strings::DFAMove(it, token.GetOriginal().begin(), token.GetOriginal().end());
  if (!it.Rejects())
    return ErrorsMade(it.PrefixErrorsMade());

  return {};
}
}  // namespace impl

bool IsStopWord(UniString const & s)
{
  /// @todo Get all common used stop words and factor out this array into
  /// search_string_utils.cpp module for example.
  static char const * arr[] = {"a", "de", "di", "da", "la", "le", "де", "ди", "да", "ла", "ля", "ле"};

  static set<UniString> const kStopWords(
      make_transform_iterator(arr, &MakeUniString),
      make_transform_iterator(arr + ARRAY_SIZE(arr), &MakeUniString));

  return kStopWords.count(s) > 0;
}

void PrepareStringForMatching(string const & name, vector<strings::UniString> & tokens)
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

string DebugPrint(NameScores scores)
{
  ostringstream os;
  os << "[ " << DebugPrint(scores.m_nameScore) << ", " << DebugPrint(scores.m_errorsMade) << ", "
     << scores.m_isAltOrOldName << " ]";
  return os.str();
}
}  // namespace search
