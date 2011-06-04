#include "categories_holder.hpp"

#include "../indexer/classificator.hpp"

#include "../coding/multilang_utf8_string.hpp"

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

size_t CategoriesHolder::LoadFromStream(istream & stream)
{
  m_categories.clear();

  State state = EParseTypes;

  string line;
  Category cat;
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
          cat.m_types.push_back(classif().GetTypeByPath(v));
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
        char langCode = StringUtf8Multilang::GetLangIndex(*iter);
        if (langCode == -1)
        {
          LOG(LWARNING, ("Invalid language code:", *iter));
          continue;
        }
        while (++iter)
        {
          cat.m_synonyms.push_back(make_pair(langCode, *iter));
        }
      }
      break;
    }
  }
  // add last category
  if (!cat.m_synonyms.empty() && !cat.m_types.empty())
    m_categories.push_back(cat);

  return m_categories.size();
}

}
