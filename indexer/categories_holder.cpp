#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "coding/reader.hpp"
#include "coding/reader_streambuf.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

using namespace std;

namespace
{
enum State
{
  EParseTypes,
  EParseLanguages
};

void AddGroupTranslationsToSynonyms(vector<string> const & groups,
                                    CategoriesHolder::GroupTranslations const & translations,
                                    vector<CategoriesHolder::Category::Name> & synonyms)
{
  for (string const & group : groups)
  {
    auto it = translations.find(group);
    if (it == translations.end())
      continue;
    for (auto & synonym : it->second)
      synonyms.push_back(synonym);
  }
}

bool ParseEmoji(CategoriesHolder::Category::Name & name)
{
  using namespace strings;

  auto const code = name.m_name;
  int c;
  if (!to_int(name.m_name.c_str() + 2, c, 16))
  {
    LOG(LWARNING, ("Bad emoji code:", code));
    return false;
  }

  name.m_name = ToUtf8(UniString(1 /* numChars */, static_cast<UniChar>(c)));

  if (IsASCIIString(ToUtf8(search::NormalizeAndSimplifyString(name.m_name))))
  {
    LOG(LWARNING, ("Bad emoji code:", code));
    return false;
  }

  return true;
}

void FillPrefixLengthToSuggest(CategoriesHolder::Category::Name & name)
{
  if (isdigit(name.m_name.front()) && name.m_name.front() != '0')
  {
    name.m_prefixLengthToSuggest = name.m_name[0] - '0';
    name.m_name = name.m_name.substr(1);
  }
  else
  {
    name.m_prefixLengthToSuggest = CategoriesHolder::Category::kEmptyPrefixLength;
  }
}

void ProcessName(CategoriesHolder::Category::Name name, vector<string> const & groups,
                 vector<uint32_t> const & types, CategoriesHolder::GroupTranslations & translations,
                 vector<CategoriesHolder::Category::Name> & synonyms)
{
  if (name.m_name.empty())
  {
    LOG(LWARNING, ("Incorrect name for category:", groups));
    return;
  }

  FillPrefixLengthToSuggest(name);

  if (strings::StartsWith(name.m_name, "U+") && !ParseEmoji(name))
    return;

  if (groups.size() == 1 && types.empty())
  {
    // Not a translation, but a category group definition.
    translations[groups[0]].push_back(name);
  }
  else
  {
    synonyms.push_back(name);
  }
}

void ProcessCategory(string const & line, vector<string> & groups, vector<uint32_t> & types)
{
  // Check if category is a group reference.
  if (line[0] == '@')
  {
    CHECK((groups.empty() || !types.empty()), ("Two groups in a group definition, line:", line));
    groups.push_back(line);
    return;
  }

  // Split category to subcategories for classificator.
  vector<string> v;
  strings::Tokenize(line, "-", base::MakeBackInsertFunctor(v));

  // Get classificator type.
  uint32_t const type = classif().GetTypeByPathSafe(v);
  if (type == 0)
  {
    LOG(LWARNING, ("Invalid type:", v, "; during parsing the line:", line));
    return;
  }

  types.push_back(type);
}
}  // namespace

// static
int8_t constexpr CategoriesHolder::kEnglishCode;
int8_t constexpr CategoriesHolder::kUnsupportedLocaleCode;
uint8_t constexpr CategoriesHolder::kMaxSupportedLocaleIndex;

// *NOTE* These constants should be updated when adding new
// translation to categories.txt. When editing, keep in mind to check
// CategoriesHolder::MapLocaleToInteger() and
// CategoriesHolder::MapIntegerToLocale() as their implementations
// strongly depend on the contents of the variable.
vector<CategoriesHolder::Mapping> const CategoriesHolder::kLocaleMapping = {{"en", 1},
                                                                            {"ru", 2},
                                                                            {"uk", 3},
                                                                            {"de", 4},
                                                                            {"fr", 5},
                                                                            {"it", 6},
                                                                            {"es", 7},
                                                                            {"ko", 8},
                                                                            {"ja", 9},
                                                                            {"cs", 10},
                                                                            {"nl", 11},
                                                                            {"zh-Hant", 12},
                                                                            {"pl", 13},
                                                                            {"pt", 14},
                                                                            {"hu", 15},
                                                                            {"th", 16},
                                                                            {"zh-Hans", 17},
                                                                            {"ar", 18},
                                                                            {"da", 19},
                                                                            {"tr", 20},
                                                                            {"sk", 21},
                                                                            {"sv", 22},
                                                                            {"vi", 23},
                                                                            {"id", 24},
                                                                            {"ro", 25},
                                                                            {"nb", 26},
                                                                            {"fi", 27},
                                                                            {"el", 28},
                                                                            {"he", 29},
                                                                            {"sw", 30},
                                                                            {"fa", 31}};
vector<string> CategoriesHolder::kDisabledLanguages = {"el", "he", "sw"};

CategoriesHolder::CategoriesHolder(unique_ptr<Reader> && reader)
{
  ReaderStreamBuf buffer(move(reader));
  istream s(&buffer);
  LoadFromStream(s);

#if defined(DEBUG)
  for (auto const & entry : kLocaleMapping)
    ASSERT_LESS_OR_EQUAL(entry.m_code, kMaxSupportedLocaleIndex, ());
#endif
}

void CategoriesHolder::AddCategory(Category & cat, vector<uint32_t> & types)
{
  if (!cat.m_synonyms.empty() && !types.empty())
  {
    shared_ptr<Category> p(new Category());
    p->Swap(cat);

    for (uint32_t const t : types)
      m_type2cat.insert(make_pair(t, p));

    for (auto const & synonym : p->m_synonyms)
    {
      auto const locale = synonym.m_locale;
      ASSERT_NOT_EQUAL(locale, kUnsupportedLocaleCode, ());

      auto const localePrefix = strings::UniString(1, static_cast<strings::UniChar>(locale));

      auto const uniName = search::NormalizeAndSimplifyString(synonym.m_name);

      vector<strings::UniString> tokens;
      SplitUniString(uniName, base::MakeBackInsertFunctor(tokens), search::Delimiters());

      for (auto const & token : tokens)
      {
        if (!ValidKeyToken(token))
          continue;
        for (uint32_t const t : types)
          m_name2type.Add(localePrefix + token, t);
      }
    }
  }

  cat.m_synonyms.clear();
  types.clear();
}

bool CategoriesHolder::ValidKeyToken(strings::UniString const & s)
{
  if (s.size() > 2)
    return true;

  /// @todo We need to have global stop words array for the most used languages.
  for (char const * token : {"a", "z", "s", "d", "di", "de", "le", "ra", "ao"})
  {
    if (s.IsEqualAscii(token))
      return false;
  }

  return true;
}

void CategoriesHolder::LoadFromStream(istream & s)
{
  m_type2cat.clear();
  m_name2type.Clear();
  m_groupTranslations.clear();

  State state = EParseTypes;
  string line;
  Category cat;
  vector<uint32_t> types;
  vector<string> currentGroups;

  int lineNumber = 0;
  while (s.good())
  {
    ++lineNumber;
    getline(s, line);
    strings::Trim(line);
    // Allow for comments starting with '#' character.
    if (!line.empty() && line[0] == '#')
      continue;

    strings::SimpleTokenizer iter(line, state == EParseTypes ? "|" : ":|");

    if (state == EParseTypes)
    {
      AddCategory(cat, types);
      currentGroups.clear();

      while (iter)
      {
        ProcessCategory(*iter, currentGroups, types);
        ++iter;
      }

      if (!types.empty() || currentGroups.size() == 1)
      {
        state = EParseLanguages;
        AddGroupTranslationsToSynonyms(currentGroups, m_groupTranslations, cat.m_synonyms);
      }
    }
    else if (state == EParseLanguages)
    {
      if (!iter)
      {
        state = EParseTypes;
        continue;
      }

      int8_t const langCode = MapLocaleToInteger(*iter);
      CHECK(langCode != kUnsupportedLocaleCode,
            ("Invalid language code:", *iter, "at line:", lineNumber));

      while (++iter)
      {
        Category::Name name;
        name.m_locale = langCode;
        name.m_name = *iter;

        ProcessName(name, currentGroups, types, m_groupTranslations, cat.m_synonyms);
      }
    }
  }
  // Add the last category.
  AddCategory(cat, types);
}

bool CategoriesHolder::GetNameByType(uint32_t type, int8_t locale, string & name) const
{
  auto const range = m_type2cat.equal_range(type);

  string enName;
  for (auto it = range.first; it != range.second; ++it)
  {
    Category const & cat = *it->second;
    for (auto const & synonym : cat.m_synonyms)
    {
      if (synonym.m_locale == locale)
      {
        name = synonym.m_name;
        return true;
      }
      else if (enName.empty() && (synonym.m_locale == kEnglishCode))
      {
        enName = synonym.m_name;
      }
    }
  }

  if (!enName.empty())
  {
    name = enName;
    return true;
  }

  return false;
}

string CategoriesHolder::GetReadableFeatureType(uint32_t type, int8_t locale) const
{
  ASSERT_NOT_EQUAL(type, 0, ());
  uint8_t level = ftype::GetLevel(type);
  ASSERT_GREATER(level, 0, ());

  uint32_t originalType = type;
  string name;
  while (true)
  {
    if (GetNameByType(type, locale, name))
      return name;

    if (--level == 0)
      break;

    ftype::TruncValue(type, level);
  }

  return classif().GetReadableObjectName(originalType);
}

bool CategoriesHolder::IsTypeExist(uint32_t type) const
{
  auto const range = m_type2cat.equal_range(type);
  return range.first != range.second;
}

// static
int8_t CategoriesHolder::MapLocaleToInteger(string const & locale)
{
  ASSERT(!kLocaleMapping.empty(), ());
  ASSERT_EQUAL(string(kLocaleMapping[0].m_name), "en", ());
  ASSERT_EQUAL(kLocaleMapping[0].m_code, kEnglishCode, ());
  ASSERT(
      find(kDisabledLanguages.begin(), kDisabledLanguages.end(), "en") == kDisabledLanguages.end(),
      ());

  for (auto const & entry : kLocaleMapping)
  {
    if (locale.find(entry.m_name) == 0)
      return entry.m_code;
  }

  // Special cases for different Chinese variations
  if (locale.find("zh") == 0)
  {
    string lower = locale;
    strings::AsciiToLower(lower);

    for (char const * s : {"hant", "tw", "hk", "mo"})
    {
      if (lower.find(s) != string::npos)
        return 12;  // Traditional Chinese
    }

    return 17;  // Simplified Chinese by default for all other cases
  }

  return kUnsupportedLocaleCode;
}

// static
string CategoriesHolder::MapIntegerToLocale(int8_t code)
{
  if (code <= 0 || static_cast<size_t>(code) > kLocaleMapping.size())
    return string();
  return kLocaleMapping[code - 1].m_name;
}
