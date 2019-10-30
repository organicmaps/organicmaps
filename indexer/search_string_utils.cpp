#include "indexer/search_string_utils.hpp"
#include "indexer/string_set.hpp"

#include "base/assert.hpp"
#include "base/dfa_helpers.hpp"
#include "base/macros.hpp"
#include "base/mem_trie.hpp"

#include "3party/utfcpp/source/utf8/unchecked.h"

#include <algorithm>
#include <memory>
#include <queue>
#include <vector>

using namespace std;
using namespace strings;

namespace search
{
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

// Replaces '#' followed by an end-of-string or a digit with space.
void RemoveNumeroSigns(UniString & s)
{
  size_t const n = s.size();

  size_t i = 0;
  while (i < n)
  {
    if (s[i] != '#')
    {
      ++i;
      continue;
    }

    size_t j = i + 1;
    while (j < n && IsASCIISpace(s[j]))
      ++j;

    if (j == n || IsASCIIDigit(s[j]))
      s[i] = ' ';

    i = j;
  }
}
}  // namespace

size_t GetMaxErrorsForTokenLength(size_t length)
{
  if (length < 4)
    return 0;
  if (length < 8)
    return 1;
  return 2;
}

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

UniString NormalizeAndSimplifyString(string const & s)
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

  // Remove accents that can appear after NFKD normalization.
  uniString.erase_if([](UniChar const & c) {
    // ̀  COMBINING GRAVE ACCENT
    // ́  COMBINING ACUTE ACCENT
    return (c == 0x0300 || c == 0x0301);
  });

  RemoveNumeroSigns(uniString);

  // Replace sequence of spaces with single one.
  auto const spacesChecker = [](UniChar lhs, UniChar rhs) { return (lhs == rhs) && (lhs == ' '); };
  uniString.erase(unique(uniString.begin(), uniString.end(), spacesChecker), uniString.end());

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

  StreetsSynonymsHolder()
  {
    char const * affics[] =
    {
      // Russian
      "аллея", "бульвар", "набережная", "переулок", "площадь", "проезд", "проспект", "шоссе", "тупик", "улица", "тракт", "ал", "бул", "наб", "пер", "пл", "пр", "просп", "ш", "туп", "ул", "тр",

      // English
      "street", "avenue", "square", "road", "boulevard", "drive", "highway", "lane", "way", "circle", "st", "av", "ave", "sq", "rd", "blvd", "dr", "hwy", "ln",

      // Lithuanian
      "g", "pr", "pl", "kel",

      // Български език - Bulgarian
      "булевард", "бул", "площад", "пл", "улица", "ул", "квартал", "кв",

      // Canada - Canada
      "allee", "alley", "autoroute", "aut", "bypass", "byway", "carrefour", "carref", "chemin", "cercle", "circle", "côte", "crossing", "cross", "expressway", "freeway", "fwy", "line", "link", "loop", "parkway", "pky", "pkwy", "path", "pathway", "ptway", "route", "rue", "rte", "trail", "walk",

      // Cesky - Czech
      "ulice", "ul", "náměstí", "nám",

      // Deutsch - German
      "allee", "al", "brücke", "br", "chaussee", "gasse", "gr", "pfad", "straße", "str", "weg", "platz",

      // Español - Spanish
      "avenida", "avd", "avda", "bulevar", "bulev", "calle", "calleja", "cllja", "callejón", "callej", "cjon", "callejuela", "cjla", "callizo", "cllzo", "calzada", "czada", "costera", "coste", "plza", "pza", "plazoleta", "pzta", "plazuela", "plzla", "tránsito", "trans", "transversal", "trval", "trasera", "tras", "travesía", "trva",

      // Français - French
      "rue", "avenue", "carré", "cercle", "route", "boulevard", "drive", "autoroute", "lane", "chemin",

       // Italiano - Italian
      "corso", "piazza", "piazzale", "strada", "via", "viale",

      // Nederlands - Dutch
      "laan", "ln.", "straat", "steenweg", "stwg", "st",

      // Norsk - Norwegian
      "vei", "veien", "vn", "gaten", "gata", "gt", "plass", "plassen", "sving", "svingen", "sv",

      // Polski - Polish
      "aleja", "aleje", "aleji", "alejach", "aleją", "plac", "placu", "placem", "ulica", "ulicy",

      // Português - Portuguese
      "street", "avenida", "quadrado", "estrada", "boulevard", "carro", "auto-estrada", "lane", "caminho",

      // Română - Romanian
      "bul", "bdul", "blv", "bulevard", "bulevardu", "calea", "cal", "piața", "pţa", "pța", "strada", "stra", "stradela", "sdla", "stradă", "unitate", "autostradă", "lane",

      // Slovenščina - Slovenian
      "cesta",

      // Suomi - Finnish
      "kaari", "kri", "katu", "kuja", "kj", "kylä", "polku", "tie", "t", "tori", "väylä", "vlä",

      // Svenska - Swedish
      "väg", "vägen", "gatan", "gränd", "gränden", "stig", "stigen", "plats", "platsen",

      // Türkçe - Turkish
      "sokak", "sk", "sok", "sokağı", "cadde", "cad", "cd", "caddesi", "bulvar", "bulvarı",

      // Tiếng Việt – Vietnamese
      "quốc lộ", "ql", "tỉnh lộ", "tl", "Đại lộ", "Đl", "Đường", "Đ", "Đường sắt", "Đs", "Đường phố", "Đp", "vuông", "con Đường", "Đại lộ", "Đường cao tốc",

      // Українська - Ukrainian
      "дорога", "провулок", "площа", "шосе", "вулиця", "дор", "пров", "вул"
    };

    for (auto const * s : affics)
    {
      UniString const us = NormalizeAndSimplifyString(s);
      m_strings.Add(us, true /* end of string */);
    }
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
  Trie m_strings;
};

StreetsSynonymsHolder g_streets;
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

UniString GetStreetNameAsKey(string const & name, bool ignoreStreetSynonyms)
{
  if (name.empty())
    return UniString();

  UniString res;
  SimpleTokenizer iter(name, kStreetTokensSeparator);
  while (iter)
  {
    UniString const s = NormalizeAndSimplifyString(*iter);
    ++iter;

    if (ignoreStreetSynonyms && IsStreetSynonym(s))
      continue;

    res.append(s);
  }

  return (res.empty() ? NormalizeAndSimplifyString(name) : res);
}

bool IsStreetSynonym(UniString const & s)
{
  return g_streets.FullMatch(s);
}

bool IsStreetSynonymPrefix(UniString const & s)
{
  return g_streets.MatchPrefix(s);
}

bool IsStreetSynonymWithMisprints(UniString const & s)
{
  auto const dfa = BuildLevenshteinDFA(s);
  return g_streets.MatchWithMisprints(dfa);
}

bool IsStreetSynonymPrefixWithMisprints(UniString const & s)
{
  auto const dfa = strings::PrefixDFAModifier<strings::LevenshteinDFA>(BuildLevenshteinDFA(s));
  return g_streets.MatchWithMisprints(dfa);
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
