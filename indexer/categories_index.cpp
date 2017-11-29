#include "categories_index.hpp"
#include "search_delimiters.hpp"
#include "search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/set.hpp"

namespace
{
void AddAllNonemptySubstrings(base::MemTrie<string, base::VectorValues<uint32_t>> & trie,
                              string const & s, uint32_t value)
{
  ASSERT(!s.empty(), ());
  for (size_t i = 0; i < s.length(); ++i)
  {
    string t;
    for (size_t j = i; j < s.length(); ++j)
    {
      t.push_back(s[j]);
      trie.Add(t, value);
    }
  }
}

template <typename TF>
void ForEachToken(string const & s, TF && fn)
{
  vector<strings::UniString> tokens;
  SplitUniString(search::NormalizeAndSimplifyString(s), MakeBackInsertFunctor(tokens),
                 search::Delimiters());
  for (auto const & token : tokens)
    fn(strings::ToUtf8(token));
}

void TokenizeAndAddAllSubstrings(base::MemTrie<string, base::VectorValues<uint32_t>> & trie,
                                 string const & s, uint32_t value)
{
  auto fn = [&](string const & token)
  {
    AddAllNonemptySubstrings(trie, token, value);
  };
  ForEachToken(s, fn);
}
}  // namespace

namespace indexer
{
void CategoriesIndex::AddCategoryByTypeAndLang(uint32_t type, int8_t lang)
{
  ASSERT(lang >= 1 && static_cast<size_t>(lang) <= CategoriesHolder::kLocaleMapping.size(),
         ("Invalid lang code:", lang));
  m_catHolder->ForEachNameByType(type, [&](TCategory::Name const & name)
                                 {
                                   if (name.m_locale == lang)
                                     TokenizeAndAddAllSubstrings(m_trie, name.m_name, type);
                                 });
}

void CategoriesIndex::AddCategoryByTypeAllLangs(uint32_t type)
{
  for (size_t i = 1; i <= CategoriesHolder::kLocaleMapping.size(); ++i)
    AddCategoryByTypeAndLang(type, i);
}

void CategoriesIndex::AddAllCategoriesInLang(int8_t lang)
{
  ASSERT(lang >= 1 && static_cast<size_t>(lang) <= CategoriesHolder::kLocaleMapping.size(),
         ("Invalid lang code:", lang));
  m_catHolder->ForEachTypeAndCategory([&](uint32_t type, TCategory const & cat)
                                      {
                                        for (auto const & name : cat.m_synonyms)
                                        {
                                          if (name.m_locale == lang)
                                            TokenizeAndAddAllSubstrings(m_trie, name.m_name, type);
                                        }
                                      });
}

void CategoriesIndex::AddAllCategoriesInAllLangs()
{
  m_catHolder->ForEachTypeAndCategory([this](uint32_t type, TCategory const & cat)
                                      {
                                        for (auto const & name : cat.m_synonyms)
                                          TokenizeAndAddAllSubstrings(m_trie, name.m_name, type);
                                      });
}

void CategoriesIndex::GetCategories(string const & query, vector<TCategory> & result) const
{
  vector<uint32_t> types;
  GetAssociatedTypes(query, types);
  my::SortUnique(types);
  m_catHolder->ForEachTypeAndCategory([&](uint32_t type, TCategory const & cat)
                                      {
                                        if (binary_search(types.begin(), types.end(), type))
                                          result.push_back(cat);
                                      });
}

void CategoriesIndex::GetAssociatedTypes(string const & query, vector<uint32_t> & result) const
{
  bool first = true;
  set<uint32_t> intersection;
  auto processToken = [&](string const & token)
  {
    set<uint32_t> types;
    auto fn = [&](string const &, uint32_t type)
    {
      types.insert(type);
    };
    m_trie.ForEachInSubtree(token, fn);

    if (first)
    {
      intersection.swap(types);
    }
    else
    {
      set<uint32_t> tmp;
      set_intersection(intersection.begin(), intersection.end(), types.begin(), types.end(),
                       inserter(tmp, tmp.begin()));
      intersection.swap(tmp);
    }
    first = false;
  };
  ForEachToken(query, processToken);

  result.insert(result.end(), intersection.begin(), intersection.end());
}
}  // namespace indexer
