#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/search_string_utils.hpp"

#include "coding/reader.hpp"
#include "coding/reader_streambuf.hpp"

#include "base/logging.hpp"


namespace
{
enum ParseState
{
  EParseTypes,
  EParseLanguages
};

void AddGroupTranslationsToSynonyms(std::vector<std::string> const & groups,
                                    CategoriesHolder::GroupTranslations const & translations,
                                    std::vector<CategoriesHolder::Category::Name> & synonyms)
{
  for (std::string const & group : groups)
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
  if (std::isdigit(name.m_name.front()) && name.m_name.front() != '0')
  {
    name.m_prefixLengthToSuggest = name.m_name[0] - '0';
    name.m_name = name.m_name.substr(1);
  }
  else
  {
    name.m_prefixLengthToSuggest = CategoriesHolder::Category::kEmptyPrefixLength;
  }
}

void ProcessName(CategoriesHolder::Category::Name name, std::vector<std::string> const & groups,
                 std::vector<uint32_t> const & types, CategoriesHolder::GroupTranslations & translations,
                 std::vector<CategoriesHolder::Category::Name> & synonyms)
{
  if (name.m_name.empty())
  {
    LOG(LWARNING, ("Incorrect name for category:", groups));
    return;
  }

  FillPrefixLengthToSuggest(name);

  if (name.m_name.starts_with("U+") && !ParseEmoji(name))
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

void ProcessCategory(std::string_view line, std::vector<std::string> & groups, std::vector<uint32_t> & types)
{
  // Check if category is a group reference.
  if (line[0] == '@')
  {
    CHECK((groups.empty() || !types.empty()), ("Two groups in a group definition, line:", line));
    groups.push_back(std::string(line));
    return;
  }

  // Get classificator type.
  uint32_t const type = classif().GetTypeByPathSafe(strings::Tokenize(line, "-"));
  if (type == Classificator::INVALID_TYPE)
  {
    LOG(LWARNING, ("Invalid type when parsing the line:", line));
    return;
  }

  types.push_back(type);
}
}  // namespace

// static
int8_t constexpr CategoriesHolder::kEnglishCode;
int8_t constexpr CategoriesHolder::kUnsupportedLocaleCode;

CategoriesHolder::CategoriesHolder(std::unique_ptr<Reader> && reader)
{
  ReaderStreamBuf buffer(std::move(reader));
  std::istream s(&buffer);
  LoadFromStream(s);

#if defined(DEBUG)
  for (auto const & entry : kLocaleMapping)
    ASSERT_LESS_OR_EQUAL(uint8_t(entry.m_code), kLocaleMapping.size(), ());
#endif
}

void CategoriesHolder::AddCategory(Category & cat, std::vector<uint32_t> & types)
{
  if (!cat.m_synonyms.empty() && !types.empty())
  {
    std::shared_ptr<Category> p(new Category());
    p->Swap(cat);

    for (uint32_t const t : types)
      m_type2cat.insert(std::make_pair(t, p));

    for (auto const & synonym : p->m_synonyms)
    {
      auto const locale = synonym.m_locale;
      ASSERT_NOT_EQUAL(locale, kUnsupportedLocaleCode, ());

      auto const localePrefix = strings::UniString(1, static_cast<strings::UniChar>(locale));

      search::ForEachNormalizedToken(synonym.m_name, [&](strings::UniString const & token)
      {
        if (ValidKeyToken(token))
        {
          for (uint32_t const t : types)
            m_name2type.Add(localePrefix + token, t);
        }
      });
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

void CategoriesHolder::LoadFromStream(std::istream & s)
{
  m_type2cat.clear();
  m_name2type.Clear();
  m_groupTranslations.clear();

  ParseState state = EParseTypes;
  std::string line;
  Category cat;
  std::vector<uint32_t> types;
  std::vector<std::string> currentGroups;

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

bool CategoriesHolder::GetNameByType(uint32_t type, int8_t locale, std::string & name) const
{
  auto const range = m_type2cat.equal_range(type);

  std::string enName;
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

std::string CategoriesHolder::GetReadableFeatureType(uint32_t type, int8_t locale) const
{
  ASSERT_NOT_EQUAL(type, 0, ());
  uint8_t level = ftype::GetLevel(type);
  ASSERT_GREATER(level, 0, ());

  uint32_t originalType = type;
  std::string name;
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
int8_t CategoriesHolder::MapLocaleToInteger(std::string_view const locale)
{
  ASSERT(!kLocaleMapping.empty(), ());
  ASSERT_EQUAL(kLocaleMapping[0].m_name, std::string_view("en"), ());
  ASSERT_EQUAL(kLocaleMapping[0].m_code, kEnglishCode, ());

  for (auto it = kLocaleMapping.crbegin(); it != kLocaleMapping.crend(); ++it)
  {
    if (locale.find(it->m_name) == 0)
      return it->m_code;
  }

  // Special cases for different Chinese variations
  if (locale.find("zh") == 0)
  {
    std::string lower(locale);
    strings::AsciiToLower(lower);

    for (char const * s : {"hant", "tw", "hk", "mo"})
    {
      if (lower.find(s) != std::string::npos)
        return kTraditionalChineseCode;
    }
    // Simplified Chinese by default for all other cases.
    return kSimplifiedChineseCode;
  }

  return kUnsupportedLocaleCode;
}

// static
std::string CategoriesHolder::MapIntegerToLocale(int8_t code)
{
  if (code <= 0 || static_cast<size_t>(code) > kLocaleMapping.size())
    return {};
  return kLocaleMapping[code - 1].m_name;
}
