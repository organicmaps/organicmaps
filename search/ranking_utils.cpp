#include "search/ranking_utils.hpp"
#include "search/token_slice.hpp"
#include "search/utils.hpp"

#include "base/dfa_helpers.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/feature_data.hpp"

#include "std/transform_iterator.hpp"

#include <algorithm>
#include <sstream>

using namespace std;
using namespace strings;

namespace search
{
namespace
{
struct TokenInfo
{
  bool m_isCategoryToken = false;
  bool m_inFeatureTypes = false;
};

UniString AsciiToUniString(char const * s) { return UniString(s, s + strlen(s)); }
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

ErrorsMade GetMinErrorsMade(vector<strings::UniString> const & tokens,
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
  static char const * arr[] = {"a", "de", "da", "la", "le"};

  static set<UniString> const kStopWords(
      make_transform_iterator(arr, &AsciiToUniString),
      make_transform_iterator(arr + ARRAY_SIZE(arr), &AsciiToUniString));

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
}  // namespace search
