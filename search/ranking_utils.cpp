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
  ForEachCategoryType(slice, locales, categories, [&](size_t i, uint32_t t)
  {
    ASSERT_LESS(i, infos.size(), ());
    auto & info = infos[i];

    info.m_isCategoryToken = true;
    if (holder.HasWithSubclass(t))
    {
      m_matchedLength += slice.Get(i).size();
      info.m_inFeatureTypes = true;
    }
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
  if (errorsMade.IsValid())
    return std::to_string(errorsMade.m_errorsMade);
  else
    return "Invalid";
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
      // Don't want to put _full_ stopwords list, not to break current ranking.
      // Only 2-letters and the most common.
      char const * arr[] = {
        "a",              // English
        "am", "im", "an", // German
        "de", "di", "da", "la", "le", // French, Spanish, Italian
        "и", "я"          // Cyrillic
      };
      for (char const * s : arr)
        m_set.insert(MakeUniString(s));
    }
    bool Has(UniString const & s) const { return m_set.count(s) > 0; }
  };

  static StopWordsChecker const swChecker;
  return swChecker.Has(s);
}

void PrepareStringForMatching(string_view name, vector<strings::UniString> & tokens)
{
  ForEachNormalizedToken(name, [&tokens](strings::UniString && token)
  {
    if (!IsStopWord(token))
      tokens.push_back(std::move(token));
  });
}

string DebugPrint(NameScore const & score)
{
  switch (score)
  {
  case NameScore::ZERO: return "Zero";
  case NameScore::SUBSTRING: return "Substring";
  case NameScore::PREFIX: return "Prefix";
  case NameScore::FULL_MATCH: return "Full Match";
  case NameScore::FULL_PREFIX: return "Full Prefix";
  case NameScore::COUNT: return "Count";
  }
  return "Unknown";
}

string DebugPrint(NameScores const & scores)
{
  ostringstream os;
  os << boolalpha << "NameScores "
     << "{ m_nameScore: " << DebugPrint(scores.m_nameScore)
     << ", m_matchedLength: " << scores.m_matchedLength
     << ", m_errorsMade: " << DebugPrint(scores.m_errorsMade)
     << ", m_isAltOrOldName: "
     << " }";
  return os.str();
}
}  // namespace search
