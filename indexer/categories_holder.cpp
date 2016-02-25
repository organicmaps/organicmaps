#include "indexer/categories_holder.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/classificator.hpp"

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


CategoriesHolder::CategoriesHolder(Reader * reader)
{
  ReaderStreamBuf buffer(reader);
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
      ASSERT(p->m_synonyms[i].m_locale != UNSUPPORTED_LOCALE_CODE, ());

      StringT const uniName = search::NormalizeAndSimplifyString(p->m_synonyms[i].m_name);

      vector<StringT> tokens;
      SplitUniString(uniName, MakeBackInsertFunctor(tokens), search::Delimiters());

      for (size_t j = 0; j < tokens.size(); ++j)
        for (size_t k = 0; k < types.size(); ++k)
          if (ValidKeyToken(tokens[j]))
            m_name2type.insert(make_pair(make_pair(p->m_synonyms[i].m_locale, tokens[j]), types[k]));
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
  for (char const * token : { "a", "z", "s", "d", "di", "de", "le", "wi", "fi", "ra", "ao" })
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

  Classificator const & c = classif();

  int lineNumber = 0;
  while (s.good())
  {
    ++lineNumber;
    getline(s, line);
    strings::SimpleTokenizer iter(line, state == EParseTypes ? "|" : ":|");

    switch (state)
    {
    case EParseTypes:
      {
        AddCategory(cat, types);

        while (iter)
        {
          // split category to sub categories for classificator
          vector<string> v;
          strings::Tokenize(*iter, "-", MakeBackInsertFunctor(v));

          // get classificator type
          uint32_t const type = c.GetTypeByPathSafe(v);
          if (type != 0)
            types.push_back(type);
          else
            LOG(LWARNING, ("Invalid type:", v, "at line:", lineNumber));

          ++iter;
        }

        if (!types.empty())
          state = EParseLanguages;
      }
      break;

    case EParseLanguages:
      {
        if (!iter)
        {
          state = EParseTypes;
          continue;
        }

        int8_t const langCode = MapLocaleToInteger(*iter);
        CHECK(langCode != UNSUPPORTED_LOCALE_CODE, ("Invalid language code:", *iter, "at line:", lineNumber));

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
            name.m_prefixLengthToSuggest = Category::EMPTY_PREFIX_LENGTH;

          // Process emoji symbols.
          using namespace strings;
          if (StartsWith(name.m_name, "U+"))
          {
            int c;
            if (!to_int(name.m_name.c_str() + 2, c, 16))
            {
              LOG(LWARNING, ("Bad emoji code:", name.m_name));
              continue;
            }

            name.m_name = ToUtf8(UniString(1, static_cast<UniChar>(c)));
          }

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
    {"en", 1 },
    {"ru", 2 },
    {"uk", 3 },
    {"de", 4 },
    {"fr", 5 },
    {"it", 6 },
    {"es", 7 },
    {"ko", 8 },
    {"ja", 9 },
    {"cs", 10 },
    {"nl", 11 },
    {"zh-Hant", 12 },
    {"pl", 13 },
    {"pt", 14 },
    {"hu", 15 },
    {"th", 16 },
    {"zh-Hans", 17 },
    {"ar", 18 },
    {"da", 19 },
    {"tr", 20 },
    {"sk", 21 },
    {"sv", 22 },
    {"vi", 23 },
    {"id", 24 },
    {"ro", 25 },
    {"nb", 26 },
    {"fi", 27 },
    {"el", 28 },
    {"he", 29 },
    {"sw", 30 }
  };
  for (size_t i = 0; i < ARRAY_SIZE(mapping); ++i)
    if (locale.find(mapping[i].m_name) == 0)
      return mapping[i].m_code;

  // Special cases for different Chinese variations
  if (locale.find("zh") == 0)
  {
    string lower = locale;
    strings::AsciiToLower(lower);

    for (char const * s : {"hant", "tw", "hk", "mo"})
      if (lower.find(s) != string::npos)
        return 12; // Traditional Chinese

    return 17; // Simplified Chinese by default for all other cases
  }

  return UNSUPPORTED_LOCALE_CODE;
}
