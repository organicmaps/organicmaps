#include "indexer/search_string_utils.hpp"

#include "indexer/transliteration_loader.hpp"

#include "coding/transliteration.hpp"

#include "base/dfa_helpers.hpp"
#include "base/mem_trie.hpp"

#include <algorithm>
#include <memory>
#include <queue>
#include <vector>

#include "3party/utfcpp/source/utf8/unchecked.h"

namespace search
{
using std::string;
using namespace strings;

namespace
{
std::vector<UniString> const kAllowedMisprints = {
    MakeUniString("ckq"),
    MakeUniString("eyjiu"),
    MakeUniString("gh"),
    MakeUniString("pf"),
    MakeUniString("vw"),
    MakeUniString("ао"),
    MakeUniString("еиэ"),
    MakeUniString("шщ"),
};

std::pair<UniString, UniString> const kPreprocessReplacements[] = {
    {MakeUniString("пр-т"),  MakeUniString("проспект")},
    {MakeUniString("пр-д"),  MakeUniString("проезд")},
    {MakeUniString("наб-я"), MakeUniString("набережная")}
};


void TransliterateHiraganaToKatakana(UniString & s)
{
  // Transliteration is heavy. Check we have any hiragana symbol before transliteration.
  if (!base::AnyOf(s, [](UniChar c){ return c >= 0x3041 && c<= 0x309F; }))
    return;

  InitTransliterationInstanceWithDefaultDirs();
  string out;
  if (Transliteration::Instance().TransliterateForce(ToUtf8(s), "Hiragana-Katakana", out))
    s = MakeUniString(out);
}
}  // namespace

size_t GetMaxErrorsForToken(UniString const & token)
{
  bool const digitsOnly = std::all_of(token.begin(), token.end(), ::isdigit);
  if (digitsOnly)
    return 0;
  return GetMaxErrorsForTokenLength(token.size());
}

LevenshteinDFA BuildLevenshteinDFA(UniString const & s)
{
  ASSERT(!s.empty(), ());
  // In search we use LevenshteinDFAs for fuzzy matching. But due to
  // performance reasons, we limit prefix misprints to fixed set of substitutions defined in
  // kAllowedMisprints and skipped letters.
  return LevenshteinDFA(s, 1 /* prefixSize */, kAllowedMisprints, GetMaxErrorsForToken(s));
}

LevenshteinDFA BuildLevenshteinDFA_Category(UniString const & s)
{
  // https://github.com/organicmaps/organicmaps/issues/3655
  // Separate DFA for categories (_Category) to avoid fancy matchings like:
  // cafe <-> care
  // ecco -> eco
  // shop <-> shoe
  // warte -> waste
  /// @todo "hote" doesn't match "hotel" now. Allow prefix search for categories?

  ASSERT(!s.empty(), ());
  return LevenshteinDFA(s, 1 /* prefixSize */, kAllowedMisprints, GetMaxErrorsForToken_Category(s.size()));
}

UniString NormalizeAndSimplifyString(std::string_view s)
{
  UniString uniString = MakeUniString(s);
  for (size_t i = 0; i < uniString.size(); ++i)
  {
    UniChar & c = uniString[i];
    switch (c)
    {
    // Replace "d with stroke" to simple d letter. Used in Vietnamese.
    // (unicode-compliant implementation leaves it unchanged)
    case 0x0110:
    case 0x0111:
      c = 'd';
      break;
    // Replace small turkish dotless 'ı' with dotted 'i'.  Our own
    // invented hack to avoid well-known Turkish I-letter bug.
    case 0x0131:
      c = 'i';
      break;
    // Replace capital turkish dotted 'İ' with dotted lowercased 'i'.
    // Here we need to handle this case manually too, because default
    // unicode-compliant implementation of MakeLowerCase converts 'İ'
    // to 'i' + 0x0307.
    case 0x0130:
      c = 'i';
      break;
    // Some Danish-specific hacks.
    case 0x00d8:  // Ø
    case 0x00f8:  // ø
      c = 'o';
      break;
    case 0x0152:  // Œ
    case 0x0153:  // œ
      c = 'o';
      uniString.insert(uniString.begin() + (i++) + 1, 'e');
      break;
    case 0x00c6:  // Æ
    case 0x00e6:  // æ
      c = 'a';
      uniString.insert(uniString.begin() + (i++) + 1, 'e');
      break;
    case 0x2116:  // №
      c = '#';
      break;
    }
  }

  MakeLowerCaseInplace(uniString);
  NormalizeInplace(uniString);
  TransliterateHiraganaToKatakana(uniString);

  // Remove accents that can appear after NFKD normalization.
  uniString.erase_if([](UniChar const & c)
  {
    // ̀  COMBINING GRAVE ACCENT
    // ́  COMBINING ACUTE ACCENT
    return (c == 0x0300 || c == 0x0301);
  });

  // Replace sequence of spaces with single one.
  uniString.erase(std::unique(uniString.begin(), uniString.end(), [](UniChar l, UniChar r)
  {
    return (l == r && l == ' ');
  }), uniString.end());

  return uniString;

  /// @todo Restore this logic to distinguish и-й in future.
  /*
  // Just after lower casing is a correct place to avoid normalization for specific chars.
  static auto const isSpecificChar = [](UniChar c) -> bool
  {
    return c == 0x0439; // й
  };
  UniString result;
  result.reserve(uniString.size());
  for (auto i = uniString.begin(), end = uniString.end(); i != end;)
  {
    auto j = find_if(i, end, isSpecificChar);
    // We don't check if (j != i) because UniString and Normalize handle it correctly.
    UniString normString(i, j);
    NormalizeInplace(normString);
    result.insert(result.end(), normString.begin(), normString.end());
    if (j == end)
      break;
    result.push_back(*j);
    i = j + 1;
  }
  return result;
  */
}

void PreprocessBeforeTokenization(UniString & query)
{
  search::Delimiters const delims;

  for (auto const & replacement : kPreprocessReplacements)
  {
    auto start = query.begin();
    while ((start = std::search(start, query.end(), replacement.first.begin(),
                                replacement.first.end())) != query.end())
    {
      auto end = start + replacement.first.size();
      if ((start == query.begin() || delims(*(start - 1))) && (end == query.end() || delims(*end)))
      {
        auto const dist = std::distance(query.begin(), start);
        query.Replace(start, end, replacement.second.begin(), replacement.second.end());
        start = query.begin() + dist;
      }
      start += 1;
    }
  }
}

UniString FeatureTypeToString(uint32_t type)
{
  string const s = "!type:" + to_string(type);
  return UniString(s.begin(), s.end());
}

std::vector<UniString> NormalizeAndTokenizeString(std::string_view s)
{
  std::vector<UniString> tokens;
  ForEachNormalizedToken(s, base::MakeBackInsertFunctor(tokens));
  return tokens;
}

bool TokenizeStringAndCheckIfLastTokenIsPrefix(std::string_view s, std::vector<UniString> & tokens)
{
  auto const uniString = NormalizeAndSimplifyString(s);

  Delimiters delims;
  SplitUniString(uniString, base::MakeBackInsertFunctor(tokens), delims);
  return !uniString.empty() && !delims(uniString.back());
}

namespace
{
char const * kStreetTokensSeparator = "\t -,.";

/// @todo Move prefixes, suffixes into separate file (autogenerated).
/// It's better to distinguish synonyms comparison according to language/region.
class StreetsSynonymsHolder
{
public:
  struct BooleanSum
  {
    using value_type = bool;

    BooleanSum() { Clear(); }

    void Add(bool value)
    {
      m_value = m_value || value;
      m_empty = false;
    }

    template <typename ToDo>
    void ForEach(ToDo && toDo) const
    {
      toDo(m_value);
    }

    void Clear()
    {
      m_value = false;
      m_empty = true;
    }

    bool Empty() const { return m_empty; }

    void Swap(BooleanSum & rhs)
    {
      std::swap(m_value, rhs.m_value);
      std::swap(m_empty, rhs.m_empty);
    }

    bool m_value;
    bool m_empty;
  };

  using Trie = base::MemTrie<UniString, BooleanSum, base::VectorMoves>;

  static StreetsSynonymsHolder const & Instance()
  {
    static const StreetsSynonymsHolder holder;
    return holder;
  }

  bool MatchPrefix(UniString const & s) const { return m_strings.HasPrefix(s); }
  bool FullMatch(UniString const & s) const { return m_strings.HasKey(s); }

  template <typename DFA>
  bool MatchWithMisprints(DFA const & dfa) const
  {
    using TrieIt = Trie::Iterator;
    using State = std::pair<TrieIt, typename DFA::Iterator>;

    auto const trieRoot = m_strings.GetRootIterator();

    std::queue<State> q;
    q.emplace(trieRoot, dfa.Begin());

    while (!q.empty())
    {
      auto const p = q.front();
      q.pop();

      auto const & currTrieIt = p.first;
      auto const & currDfaIt = p.second;

      if (currDfaIt.Accepts() && !currTrieIt.GetValues().Empty())
        return true;

      currTrieIt.ForEachMove([&q, &currDfaIt](UniChar const & c, TrieIt const & nextTrieIt)
      {
        auto nextDfaIt = currDfaIt;
        nextDfaIt.Move(c);
        DFAMove(nextDfaIt, nextTrieIt.GetLabel());
        if (!nextDfaIt.Rejects())
          q.emplace(nextTrieIt, nextDfaIt);
      });
    }

    return false;
  }

private:
  // Keep only *very-common-used* synonyms here (can increase search index, otherwise).
  // Too many synonyms increases entropy only and produces messy results ..
  StreetsSynonymsHolder()
  {
    char const * affics[] =
    {
      // Russian - Русский
      "улица", "ул",

      // English - English
      "street", "st", "road", "rd", "drive", "dr", "lane", "ln", "avenue", "av",

      // Belarusian - Беларуская мова
      "вуліца", "вул",

      // Arabic
      "شارع",

      // Armenian
      "փողոց",

      // Catalan language (Barcelona, Valencia, ...)
      "carrer",

      // Croatian - Hrvatski
      "ulica",  // Also common used transcription from RU

      // French - Français
      "rue",

      // Georgia
      "ქუჩა",

      // German - Deutsch
      "straße", "str",

      // Hungarian - Magyar
      "utca", "út",

      // Indonesia
      "jalan",

      // Italian - Italiano
      "via",

      /// @todo Also expect that this synonyms should be in categories.txt list, but we dont support lt, lv langs now.
      /// @{
      // Latvian - Latviešu
      "iela",
      // Lithuanian - Lietuvių
      "gatvė", "g.",
      ///@}

      // Portuguese - Português
      "rua",

      // Romanian - Română (Moldova)
      "strada",

      // Spanish - Español
      "calle", "avenida",

      // Turkish - Türkçe
      "sokağı", "sokak", "sk",

      // Ukrainian - Українська
      "вулиця", "вул",

      // Vietnamese - Tiếng Việt
      "đường",
    };

    for (auto const * s : affics)
      m_strings.Add(NormalizeAndSimplifyString(s), true /* end of string */);
  }

  Trie m_strings;
};

}  // namespace

string DropLastToken(string const & str)
{
  search::Delimiters delims;
  using Iter = utf8::unchecked::iterator<string::const_iterator>;

  // Find start iterator of prefix in input query.
  Iter iter(str.end());
  while (iter.base() != str.begin())
  {
    Iter prev = iter;
    --prev;

    if (delims(*prev))
      break;

    iter = prev;
  }

  return string(str.begin(), iter.base());
}

UniString GetStreetNameAsKey(std::string_view name, bool ignoreStreetSynonyms)
{
  if (name.empty())
    return UniString();

  UniString res;
  Tokenize(name, kStreetTokensSeparator, [&](std::string_view v)
  {
    UniString const s = NormalizeAndSimplifyString(v);
    if (!ignoreStreetSynonyms || !IsStreetSynonym(s))
      res.append(s);
  });

  return (res.empty() ? NormalizeAndSimplifyString(name) : res);
}

bool IsStreetSynonym(UniString const & s) { return StreetsSynonymsHolder::Instance().FullMatch(s); }

bool IsStreetSynonymPrefix(UniString const & s)
{
  return StreetsSynonymsHolder::Instance().MatchPrefix(s);
}

bool IsStreetSynonymWithMisprints(UniString const & s)
{
  auto const dfa = BuildLevenshteinDFA(s);
  return StreetsSynonymsHolder::Instance().MatchWithMisprints(dfa);
}

bool IsStreetSynonymPrefixWithMisprints(UniString const & s)
{
  auto const dfa = PrefixDFAModifier<LevenshteinDFA>(BuildLevenshteinDFA(s));
  return StreetsSynonymsHolder::Instance().MatchWithMisprints(dfa);
}

bool ContainsNormalized(string const & str, string const & substr)
{
  UniString const ustr = NormalizeAndSimplifyString(str);
  UniString const usubstr = NormalizeAndSimplifyString(substr);
  return std::search(ustr.begin(), ustr.end(), usubstr.begin(), usubstr.end()) != ustr.end();
}

// StreetTokensFilter ------------------------------------------------------------------------------
void StreetTokensFilter::Put(UniString const & token, bool isPrefix, size_t tag)
{
  if (isPrefix)
  {
    if (m_withMisprints)
    {
      if (IsStreetSynonymPrefixWithMisprints(token))
        return;
    }
    else
    {
      if (IsStreetSynonymPrefix(token))
        return;
    }
  }
  else
  {
    if (m_withMisprints)
    {
      if (IsStreetSynonymWithMisprints(token))
        return;
    }
    else
    {
      if (IsStreetSynonym(token))
        return;
    }
  }

  m_callback(token, tag);
}
}  // namespace search
