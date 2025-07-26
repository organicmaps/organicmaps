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

// CategoriesInfo ----------------------------------------------------------------------------------
CategoriesInfo::CategoriesInfo(feature::TypesHolder const & holder, TokenSlice const & tokens, Locales const & locales,
                               CategoriesHolder const & categories)
{
  struct TokenInfo
  {
    bool m_isCategoryToken = false;
    bool m_inFeatureTypes = false;
  };

  QuerySlice slice(tokens);
  vector<TokenInfo> infos(slice.Size());

  ForEachCategoryType(slice, locales, categories, [&](size_t i, uint32_t t)
  {
    ASSERT_LESS(i, infos.size(), ());
    auto & info = infos[i];

    info.m_isCategoryToken = true;
    if (holder.HasWithSubclass(t))
      info.m_inFeatureTypes = true;
  });

  for (size_t i = 0; i < slice.Size(); ++i)
    if (infos[i].m_inFeatureTypes)
      m_matchedLength += slice.Get(i).size();

  // Note that m_inFeatureTypes implies m_isCategoryToken.

  m_pureCategories = all_of(infos.begin(), infos.end(), [](TokenInfo const & info)
  {
    ASSERT(!info.m_inFeatureTypes || info.m_isCategoryToken, ());
    return info.m_inFeatureTypes;
  });

  m_falseCategories = all_of(infos.begin(), infos.end(),
                             [](TokenInfo const & info) { return !info.m_inFeatureTypes && info.m_isCategoryToken; });
}

// ErrorsMade --------------------------------------------------------------------------------------
string DebugPrint(ErrorsMade const & errorsMade)
{
  if (errorsMade.IsValid())
    return std::to_string(errorsMade.m_errorsMade);
  else
    return "Invalid";
}

namespace impl
{
ErrorsMade GetErrorsMade(QueryParams::Token const & token, strings::UniString const & text, LevenshteinDFA const & dfa)
{
  if (token.AnyOfSynonyms([&text](strings::UniString const & s) { return text == s; }))
    return ErrorsMade(0);

  auto it = dfa.Begin();
  strings::DFAMove(it, token.GetOriginal().begin(), token.GetOriginal().end());
  if (it.Accepts())
    return ErrorsMade(it.ErrorsMade());

  return {};
}

ErrorsMade GetPrefixErrorsMade(QueryParams::Token const & token, strings::UniString const & text,
                               LevenshteinDFA const & dfa)
{
  if (token.AnyOfSynonyms([&text](strings::UniString const & s) { return StartsWith(text, s); }))
    return ErrorsMade(0);

  auto it = dfa.Begin();
  strings::DFAMove(it, token.GetOriginal().begin(), token.GetOriginal().end());
  if (!it.Rejects())
    return ErrorsMade(it.PrefixErrorsMade());

  return {};
}
}  // namespace impl

bool IsStopWord(UniString const & s)
{
  /// @todo Get all common used stop words and take out this array into search_string_utils.cpp module for example.
  /// Should skip this tokens when building search index?
  class StopWordsChecker
  {
    set<UniString> m_set;

  public:
    StopWordsChecker()
    {
      // Don't want to put _full_ stopwords list, not to break current ranking.
      // Only 2-letters and the most common.
      char const * arr[] = {
          "a",  "s",  "the",                          // English
          "am", "im", "an",                           // German
          "d",  "da", "de",  "di", "du", "la", "le",  // French, Spanish, Italian
          "и",  "я"                                   // Cyrillic
      };
      for (char const * s : arr)
        m_set.insert(MakeUniString(s));
    }
    bool Has(UniString const & s) const { return m_set.count(s) > 0; }
  };

  static StopWordsChecker const swChecker;
  return swChecker.Has(s);
}

TokensVector::TokensVector(string_view name)
{
  ForEachNormalizedToken(name, [this](strings::UniString && token)
  {
    if (!IsStopWord(token))
      m_tokens.push_back(std::move(token));
  });

  Init();
}

string DebugPrint(NameScore const & score)
{
  switch (score)
  {
  case NameScore::ZERO: return "Zero";
  case NameScore::SUBSTRING: return "Substring";
  case NameScore::PREFIX: return "Prefix";
  case NameScore::FIRST_MATCH: return "First Match";
  case NameScore::FULL_PREFIX: return "Full Prefix";
  case NameScore::FULL_MATCH: return "Full Match";
  case NameScore::COUNT: return "Count";
  }
  return "Unknown";
}

string DebugPrint(NameScores const & scores)
{
  ostringstream os;
  os << boolalpha << "NameScores "
     << "{ m_nameScore: " << DebugPrint(scores.m_nameScore) << ", m_matchedLength: " << scores.m_matchedLength
     << ", m_errorsMade: " << DebugPrint(scores.m_errorsMade) << ", m_isAltOrOldName: "
     << " }";
  return os.str();
}
}  // namespace search
