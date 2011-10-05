#include "categories_holder.hpp"

#include "../indexer/classificator.hpp"

#include "../coding/multilang_utf8_string.hpp"
#include "../coding/reader.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"


namespace search
{

struct Splitter
{
  vector<string> & m_v;
  Splitter(vector<string> & v) : m_v(v) {}
  void operator()(string const & s)
  {
    m_v.push_back(s);
  }
};

enum State {
  EParseTypes,
  EParseLanguages
};

size_t CategoriesHolder::LoadFromStream(string const & buffer)
{
  m_categories.clear();

  State state = EParseTypes;

  string line;
  Category cat;

  Classificator const & c = classif();

  istringstream stream(buffer);
  while (stream.good())
  {
    getline(stream, line);
    strings::SimpleTokenizer iter(line, ":|");

    switch (state)
    {
    case EParseTypes:
      {
        if (!cat.m_synonyms.empty() && !cat.m_types.empty())
          m_categories.push_back(cat);
        cat.m_synonyms.clear();
        cat.m_types.clear();
        while (iter)
        {
          // split category to sub categories for classificator
          vector<string> v;
          strings::Tokenize(*iter, "-", Splitter(v));
          // get classificator type
          cat.m_types.push_back(c.GetTypeByPath(v));
          ++iter;
        }
        if (!cat.m_types.empty())
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
        int8_t langCode = StringUtf8Multilang::GetLangIndex(*iter);
        if (langCode == -1)
        {
          LOG(LWARNING, ("Invalid language code:", *iter));
          continue;
        }
        while (++iter)
        {
          Category::Name name;
          name.m_lang = langCode;
          name.m_name = *iter;

          // ASSERT(name.m_Name.empty(), ());
          if (name.m_name.empty())
            continue;

          if (name.m_name[0] >= '0' && name.m_name[0] <= '9')
          {
            name.m_prefixLengthToSuggest = name.m_name[0] - '0';
            name.m_name = name.m_name.substr(1);
          }
          else
            name.m_prefixLengthToSuggest = 10;

          cat.m_synonyms.push_back(name);
        }
      }
      break;
    }
  }

  // add last category
  if (!cat.m_synonyms.empty() && !cat.m_types.empty())
    m_categories.push_back(cat);

  LOG(LINFO, ("Categories loaded: ", m_categories.size()));
  return m_categories.size();
}

void CategoriesHolder::swap(CategoriesHolder & o)
{
  m_categories.swap(o.m_categories);
}

CategoriesHolder::CategoriesHolder()
{
}

CategoriesHolder::CategoriesHolder(Reader const & reader)
{
  string buffer;
  reader.ReadAsString(buffer);
  LoadFromStream(buffer);
}

}
