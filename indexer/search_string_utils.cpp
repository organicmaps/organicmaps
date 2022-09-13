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
using namespace std;
using namespace strings;

namespace
{
vector<strings::UniString> const kAllowedMisprints = {
    strings::MakeUniString("ckq"),
    strings::MakeUniString("eyjiu"),
    strings::MakeUniString("gh"),
    strings::MakeUniString("pf"),
    strings::MakeUniString("vw"),
    strings::MakeUniString("ао"),
    strings::MakeUniString("еиэ"),
    strings::MakeUniString("шщ"),
};

void TransliterateHiraganaToKatakana(UniString & s)
{
  // Transliteration is heavy. Check we have any hiragana symbol before transliteration.
  if (!base::AnyOf(s, [](UniChar c){ return c >= 0x3041 && c<= 0x309F; }))
    return;

  InitTransliterationInstanceWithDefaultDirs();
  string out;
  if (Transliteration::Instance().TransliterateForce(strings::ToUtf8(s), "Hiragana-Katakana", out))
    s = MakeUniString(out);
}
}  // namespace

size_t GetMaxErrorsForToken(strings::UniString const & token)
{
  bool const digitsOnly = all_of(token.begin(), token.end(), ::isdigit);
  if (digitsOnly)
    return 0;
  return GetMaxErrorsForTokenLength(token.size());
}

strings::LevenshteinDFA BuildLevenshteinDFA(strings::UniString const & s)
{
  // In search we use LevenshteinDFAs for fuzzy matching. But due to
  // performance reasons, we limit prefix misprints to fixed set of substitutions defined in
  // kAllowedMisprints and skipped letters.
  return strings::LevenshteinDFA(s, 1 /* prefixSize */, kAllowedMisprints, GetMaxErrorsForToken(s));
}

UniString NormalizeAndSimplifyString(string_view s)
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
  uniString.erase(unique(uniString.begin(), uniString.end(), [](UniChar l, UniChar r)
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

void PreprocessBeforeTokenization(strings::UniString & query)
{
  search::Delimiters const delims;
  vector<pair<strings::UniString, strings::UniString>> const replacements = {
      {MakeUniString("пр-т"),  MakeUniString("проспект")},
      {MakeUniString("пр-д"),  MakeUniString("проезд")},
      {MakeUniString("наб-я"), MakeUniString("набережная")}};

  for (auto const & replacement : replacements)
  {
    auto start = query.begin();
    while ((start = std::search(start, query.end(), replacement.first.begin(),
                                replacement.first.end())) != query.end())
    {
      auto end = start + replacement.first.size();
      if ((start == query.begin() || delims(*(start - 1))) && (end == query.end() || delims(*end)))
      {
        auto const dist = distance(query.begin(), start);
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

std::vector<strings::UniString> NormalizeAndTokenizeString(std::string_view s)
{
  std::vector<strings::UniString> tokens;
  ForEachNormalizedToken(s, base::MakeBackInsertFunctor(tokens));
  return tokens;
}

bool TokenizeStringAndCheckIfLastTokenIsPrefix(std::string_view s, std::vector<strings::UniString> & tokens)
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
      swap(m_value, rhs.m_value);
      swap(m_empty, rhs.m_empty);
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
    using State = pair<TrieIt, typename DFA::Iterator>;

    auto const trieRoot = m_strings.GetRootIterator();

    queue<State> q;
    q.emplace(trieRoot, dfa.Begin());

    while (!q.empty())
    {
      auto const p = q.front();
      q.pop();

      auto const & currTrieIt = p.first;
      auto const & currDfaIt = p.second;

      if (currDfaIt.Accepts())
        return true;

      currTrieIt.ForEachMove([&q, &currDfaIt](UniChar const & c, TrieIt const & nextTrieIt) {
        auto nextDfaIt = currDfaIt;
        nextDfaIt.Move(c);
        strings::DFAMove(nextDfaIt, nextTrieIt.GetLabel());
        if (!nextDfaIt.Rejects())
          q.emplace(nextTrieIt, nextDfaIt);
      });
    }

    return false;
  }

private:
  StreetsSynonymsHolder()
  {
    char const * affics[] =
    {
      // Russian - Русский
      "аллея", "бульвар", "набережная", "переулок", "площадь", "проезд", "проспект", "шоссе", "тупик", "улица", "тракт", "ал", "бул", "наб", "пер", "пл", "пр", "просп", "ш", "туп", "ул", "тр",

      // English - English
      "street", "st", "avenue", "av", "ave", "square", "sq", "road", "rd", "boulevard", "blvd", "drive", "dr", "highway", "hwy", "lane", "ln", "way", "circle", "place", "pl",

      // Belarusian - Беларуская мова
      "вуліца", "вул", "завулак", "набярэжная", "плошча", "пл", "праезд", "праспект", "пр", "тракт", "тр", "тупік",

      // Bulgarian - Български
      "булевард", "бул", "площад", "пл", "улица", "ул", "квартал", "кв",

      /// @todo Do not use popular POI (carrefour) or Street name (rambla) tokens as generic street synonyms.
      /// This POIs (Carrefour supermarket) and Streets (La Rambla - most popular street in Barcelona)
      /// will be lost in search results, otherwise.
      /// Should reconsider candidates fetching and sorting logic from scratch to make correct processing.

      // Canada
      "allee", "alley", "autoroute", "aut", "bypass", "byway", /*"carrefour", "carref",*/ "côte", "expressway", "freeway", "fwy", "pky", "pkwy",
      /// @todo Do not use next _common search_ (e.g. 'park' is a prefix of 'parkway') tokens as generic street synonyms.
      /// Should reconsider streets matching logic to get this synonyms back.
      //"line", "link", "loop", "parkway", "path", "pathway", "route", "trail", "walk",

      // Catalan language (Barcelona, Valencia, ...)
      "avinguda", "carrer", /*"rambla", "ronda",*/ "passeig", "passatge", "travessera",

      // Croatian - Hrvatski
      "šetalište", "trg", "ulica", "ul", "poljana",

      // Czech - Čeština
      "ulice", "ul", "náměstí", "nám", "nábřeží", "nábr",

      // Danish - Dansk
      "plads", "alle", "gade",

      // Dutch - Nederlands
      "laan", "ln.", "straat", "steenweg", "stwg", "st",

      // Estonian - Eesti
      "maantee", "mnt", "puiestee", "tee", "pst",

      // Finnish - Suomi
      "kaari", "kri", "katu", "kuja", "kj", "kylä", "polku", "tie", "t", "tori", "väylä", "vlä",

      // French - Français
      "rue", "avenue", "carré", "cercle", "route", "boulevard", "drive", "autoroute", "lane", "chemin",

      // German - Deutsch
      "allee", "al", "brücke", "br", "chaussee", "gasse", "gr", "pfad", "straße", "str", "weg", "platz",

      // Hungarian - Magyar
      "utca", "út", "u.", "tér", "körút", "krt.", "rakpart", "rkp.",

       // Italian - Italiano
      "corso", "piazza", "piazzale", "strada", "via", "viale", "calle", "fondamenta",

      // Latvian - Latviešu
      "iela", "laukums",

      // Lithuanian - Lietuvių
      "gatvė", "g.", "aikštė", "a", "prospektas", "pr.", "pl", "kel",

      // Nepalese - नेपाली
      "मार्ग", "marg",

      // Norwegian - Norsk
      "vei", "veien", "vn", "gaten", "gata", "gt", "plass", "plassen", "sving", "svingen", "sv",

      // Polish - Polski
      "aleja", "aleje", "aleji", "alejach", "aleją", "plac", "placu", "placem", "ulica", "ulicy",

      // Portuguese - Português
      "rua", "r.", "travessa", "tr.", "praça", "pç.", "avenida", "quadrado", "estrada", "boulevard", "carro", "auto-estrada", "lane", "caminho",

      // Romanian - Română
      "bul", "bdul", "blv", "bulevard", "bulevardu", "calea", "cal", "piața", "pţa", "pța", "strada", "stra", "stradela", "sdla", "stradă", "unitate", "autostradă", "lane",

      // Slovenian - Slovenščina
      "cesta", "ulica", "trg", "nabrežje",

      // Spanish - Español
      "avenida", "avd", "avda", "bulevar", "bulev", "calle", "calleja", "cllja", "callejón", "callej", "cjon", "callejuela", "cjla", "callizo", "cllzo", "calzada", "czada", "costera", "coste", "plza", "pza", "plazoleta", "pzta", "plazuela", "plzla", "tránsito", "trans", "transversal", "trval", "trasera", "tras", "travesía", "trva", "paseo", "plaça",

      // Swedish - Svenska
      "väg", "vägen", "gatan", "gränd", "gränden", "stig", "stigen", "plats", "platsen",

      // Turkish - Türkçe
      "sokak", "sk.", "sok", "sokağı", "cadde", "cad", "cd", "caddesi", "bulvar", "bulvarı", "blv.",

      // Ukrainian - Українська
      "дорога", "провулок", "площа", "шосе", "вулиця", "дор", "пров", "вул",

      // Vietnamese - Tiếng Việt
      "quốc lộ", "ql", "tỉnh lộ", "tl", "Đại lộ", "Đl", "Đường", "Đ", "Đường sắt", "Đs", "Đường phố", "Đp", "vuông", "con Đường", "Đại lộ", "Đường cao tốc",
    };

    for (auto const * s : affics)
    {
      UniString const us = NormalizeAndSimplifyString(s);
      m_strings.Add(us, true /* end of string */);
    }
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

UniString GetStreetNameAsKey(string_view name, bool ignoreStreetSynonyms)
{
  if (name.empty())
    return UniString();

  UniString res;
  Tokenize(name, kStreetTokensSeparator, [&](string_view v)
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
  auto const dfa = strings::PrefixDFAModifier<strings::LevenshteinDFA>(BuildLevenshteinDFA(s));
  return StreetsSynonymsHolder::Instance().MatchWithMisprints(dfa);
}

bool ContainsNormalized(string const & str, string const & substr)
{
  UniString const ustr = NormalizeAndSimplifyString(str);
  UniString const usubstr = NormalizeAndSimplifyString(substr);
  return std::search(ustr.begin(), ustr.end(), usubstr.begin(), usubstr.end()) != ustr.end();
}

// StreetTokensFilter ------------------------------------------------------------------------------
void StreetTokensFilter::Put(strings::UniString const & token, bool isPrefix, size_t tag)
{
  using IsStreetChecker = std::function<bool(strings::UniString const &)>;

  IsStreetChecker isStreet = m_withMisprints ? IsStreetSynonymWithMisprints : IsStreetSynonym;
  IsStreetChecker isStreetPrefix =
      m_withMisprints ? IsStreetSynonymPrefixWithMisprints : IsStreetSynonymPrefix;

  auto const isStreetSynonym = isStreet(token);
  if ((isPrefix && isStreetPrefix(token)) || (!isPrefix && isStreetSynonym))
  {
    ++m_numSynonyms;
    if (m_numSynonyms == 1)
    {
      m_delayedToken = token;
      m_delayedTag = tag;
      return;
    }

    // Do not emit delayed token for incomplete street synonym.
    if ((!isPrefix || isStreetSynonym) && m_numSynonyms == 2)
      EmitToken(m_delayedToken, m_delayedTag);
  }
  EmitToken(token, tag);
}
}  // namespace search
