#include "search/ranking_utils.hpp"

#include "search/token_slice.hpp"
#include "search/utils.hpp"

#include "base/dfa_helpers.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/feature_data.hpp"

#include <algorithm>
#include <sstream>

namespace search
{
using namespace std;
using namespace strings;

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
  ForEachCategoryType(slice, locales, categories, [&holder, &infos](size_t i, uint32_t t)
  {
    ASSERT_LESS(i, infos.size(), ());
    auto & info = infos[i];

    info.m_isCategoryToken = true;
    if (holder.Has(t))
      info.m_inFeatureTypes = true;
  });

  // Note that m_inFeatureTypes implies m_isCategoryToken.

  m_pureCategories = all_of(infos.begin(), infos.end(), [](TokenInfo const & info)
  {
    ASSERT(!info.m_inFeatureTypes || info.m_isCategoryToken, ());
    return info.m_inFeatureTypes;
  });

  m_falseCategories = all_of(infos.begin(), infos.end(), [](TokenInfo const & info)
  {
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
  /// @todo Get all common used stop words and take out this array into
  /// search_string_utils.cpp module for example.
  class StopWordsChecker
  {
    set<UniString> m_set;
  public:
    StopWordsChecker()
    {
      /// @todo I understand latin words logic, but where did "де", "ди", "да" ... come from ???
      /// "и" (й), "я" is more relevant here.
      for (char const * s : {"a", "de", "di", "da", "la", "le", "де", "ди", "да", "ла", "ля", "ле"})
        m_set.insert(MakeUniString(s));
    }
    bool Has(UniString const & s) const { return m_set.count(s) > 0; }
  };

  static StopWordsChecker const swChecker;
  return swChecker.Has(s);
}

void PrepareStringForMatching(string_view name, vector<strings::UniString> & tokens)
{
  SplitUniString(NormalizeAndSimplifyString(name), [&tokens](strings::UniString const & token)
  {
    if (!IsStopWord(token))
      tokens.push_back(token);
  }, Delimiters());
}

string DebugPrint(NameScore const & score)
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

string DebugPrint(NameScores const & scores)
{
  ostringstream os;
  os << "[ " << DebugPrint(scores.m_nameScore) << ", Length:" << scores.m_matchedLength << ", " << DebugPrint(scores.m_errorsMade) << ", "
     << (scores.m_isAltOrOldName ? "Old name" : "New name") << " ]";
  return os.str();
}
}  // namespace search
