#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "coding/reader.hpp"
#include "coding/reader_streambuf.hpp"

#include "base/logging.hpp"
#include "base/stl_add.hpp"

namespace
{
enum State
{
  EParseTypes,
  EParseLanguages
};

}  // unnamed namespace

// static
size_t const CategoriesHolder::kNumLanguages = 30;
size_t const CategoriesHolder::kEnglishCode = 1;

CategoriesHolder::CategoriesHolder(unique_ptr<Reader> && reader)
{
  ReaderStreamBuf buffer(move(reader));
  istream s(&buffer);
  LoadFromStream(s);
}

void CategoriesHolder::AddCategory(Category & cat, vector<uint32_t> & types)
{
  if (!cat.m_synonyms.empty() && !types.empty())
  {
    shared_ptr<Category> p(new Category());
    p->Swap(cat);

    for (size_t i = 0; i < types.size(); ++i)
      m_type2cat.insert(make_pair(types[i], p));

    for (size_t i = 0; i < p->m_synonyms.size(); ++i)
    {
      ASSERT(p->m_synonyms[i].m_locale != kUnsupportedLocaleCode, ());

      StringT const uniName = search::NormalizeAndSimplifyString(p->m_synonyms[i].m_name);

      vector<StringT> tokens;
      SplitUniString(uniName, MakeBackInsertFunctor(tokens), search::Delimiters());

      for (size_t j = 0; j < tokens.size(); ++j)
        for (size_t k = 0; k < types.size(); ++k)
          if (ValidKeyToken(tokens[j]))
            m_name2type.insert(
                make_pair(make_pair(p->m_synonyms[i].m_locale, tokens[j]), types[k]));
    }
  }

  cat.m_synonyms.clear();
  types.clear();
}

bool CategoriesHolder::ValidKeyToken(StringT const & s)
{
  if (s.size() > 2)
    return true;

  /// @todo We need to have global stop words array for the most used languages.
  for (char const * token : {"a", "z", "s", "d", "di", "de", "le", "wi", "fi", "ra", "ao"})
    if (s.IsEqualAscii(token))
      return false;

  return true;
}

void CategoriesHolder::LoadFromStream(istream & s)
{
  m_type2cat.clear();
  m_name2type.clear();

  State state = EParseTypes;
  string line;

  Category cat;
  vector<uint32_t> types;
  vector<string> currentGroups;
  multimap<string, Category::Name> groupTranslations;

  Classificator const & c = classif();

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

    switch (state)
    {
    case EParseTypes:
    {
      AddCategory(cat, types);
      currentGroups.clear();

      while (iter)
      {
        // Check if category is a group reference.
        if ((*iter)[0] == '@')
        {
          CHECK((currentGroups.empty() || !types.empty()),
                ("Two groups in a group definition at line", lineNumber));
          currentGroups.push_back(*iter);
        }
        else
        {
          // Split category to subcategories for classificator.
          vector<string> v;
          strings::Tokenize(*iter, "-", MakeBackInsertFunctor(v));

          // Get classificator type.
          uint32_t const type = c.GetTypeByPathSafe(v);
          if (type != 0)
            types.push_back(type);
          else
            LOG(LWARNING, ("Invalid type:", v, "at line:", lineNumber));
        }

        ++iter;
      }

      if (!types.empty() || currentGroups.size() == 1)
        state = EParseLanguages;
    }
    break;

    case EParseLanguages:
    {
      if (!iter)
      {
        // If the category groups are specified, add translations from them.

        ///@todo According to the current logic, categories.txt should have
        /// the blank string at the end of file.
        if (!types.empty())
        {
          for (string const & group : currentGroups)
          {
            auto trans = groupTranslations.equal_range(group);
            for (auto it = trans.first; it != trans.second; ++it)
              cat.m_synonyms.push_back(it->second);
          }
        }

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

        if (name.m_name.empty())
        {
          LOG(LWARNING, ("Empty category name at line:", lineNumber));
          continue;
        }

        if (name.m_name[0] >= '0' && name.m_name[0] <= '9')
        {
          name.m_prefixLengthToSuggest = name.m_name[0] - '0';
          name.m_name = name.m_name.substr(1);
        }
        else
          name.m_prefixLengthToSuggest = Category::kEmptyPrefixLength;

        // Process emoji symbols.
        using namespace strings;
        if (StartsWith(name.m_name, "U+"))
        {
          auto const code = name.m_name;
          int c;
          if (!to_int(name.m_name.c_str() + 2, c, 16))
          {
            LOG(LWARNING, ("Bad emoji code:", code));
            continue;
          }

          name.m_name = ToUtf8(UniString(1, static_cast<UniChar>(c)));

          if (IsASCIIString(ToUtf8(search::NormalizeAndSimplifyString(name.m_name))))
          {
            LOG(LWARNING, ("Bad emoji code:", code));
            continue;
          }
        }

        if (currentGroups.size() == 1 && types.empty())
        {
          // Not a translation, but a category group definition
          groupTranslations.emplace(currentGroups[0], name);
        }
        else
          cat.m_synonyms.push_back(name);
      }
    }
    break;
    }
  }

  // add last category
  AddCategory(cat, types);
}

bool CategoriesHolder::GetNameByType(uint32_t type, int8_t locale, string & name) const
{
  pair<IteratorT, IteratorT> const range = m_type2cat.equal_range(type);

  for (IteratorT i = range.first; i != range.second; ++i)
  {
    Category const & cat = *i->second;
    for (size_t j = 0; j < cat.m_synonyms.size(); ++j)
      if (cat.m_synonyms[j].m_locale == locale)
      {
        name = cat.m_synonyms[j].m_name;
        return true;
      }
  }

  if (range.first != range.second)
  {
    name = range.first->second->m_synonyms[0].m_name;
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
  pair<IteratorT, IteratorT> const range = m_type2cat.equal_range(type);
  return range.first != range.second;
}

int8_t CategoriesHolder::MapLocaleToInteger(string const & locale)
{
  struct Mapping
  {
    char const * m_name;
    int8_t m_code;
  };
  // TODO(AlexZ): These constants should be updated when adding new
  // translation into categories.txt
  static const Mapping mapping[] = {
      {"en", 1},  {"ru", 2},  {"uk", 3},  {"de", 4},  {"fr", 5},       {"it", 6},
      {"es", 7},  {"ko", 8},  {"ja", 9},  {"cs", 10}, {"nl", 11},      {"zh-Hant", 12},
      {"pl", 13}, {"pt", 14}, {"hu", 15}, {"th", 16}, {"zh-Hans", 17}, {"ar", 18},
      {"da", 19}, {"tr", 20}, {"sk", 21}, {"sv", 22}, {"vi", 23},      {"id", 24},
      {"ro", 25}, {"nb", 26}, {"fi", 27}, {"el", 28}, {"he", 29},      {"sw", 30}};
  static_assert(ARRAY_SIZE(mapping) == kNumLanguages, "");
  static_assert(CategoriesHolder::kEnglishCode == 1, "");
  ASSERT_EQUAL(string(mapping[0].m_name), "en", ());
  ASSERT_EQUAL(mapping[0].m_code, CategoriesHolder::kEnglishCode, ());
  for (size_t i = 0; i < kNumLanguages; ++i)
  {
    if (locale.find(mapping[i].m_name) == 0)
      return mapping[i].m_code;
  }

  // Special cases for different Chinese variations
  if (locale.find("zh") == 0)
  {
    string lower = locale;
    strings::AsciiToLower(lower);

    for (char const * s : {"hant", "tw", "hk", "mo"})
      if (lower.find(s) != string::npos)
        return 12;  // Traditional Chinese

    return 17;  // Simplified Chinese by default for all other cases
  }

  return kUnsupportedLocaleCode;
}
