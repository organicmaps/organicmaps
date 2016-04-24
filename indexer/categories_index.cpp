#include "categories_index.hpp"

#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/set.hpp"

namespace
{
void AddAllSubstrings(my::MemTrie<string, uint32_t> & trie, string const & s, uint32_t value)
{
  for (size_t i = 0; i < s.length(); ++i)
  {
    string t;
    for (size_t j = i; j < s.length(); ++j)
    {
      t.append(1, s[j]);
      trie.Add(t, value);
    }
  }
}
}  // namespace

namespace indexer
{
void CategoriesIndex::AddCategoryByTypeAndLang(uint32_t type, int8_t lang)
{
  m_catHolder.ForEachNameByType(type, [&](CategoriesHolder::Category::Name const & name)
                                {
                                  if (name.m_locale == lang)
                                    AddAllSubstrings(m_trie, name.m_name, type);
                                });
}

void CategoriesIndex::AddCategoryByTypeAllLangs(uint32_t type)
{
  for (size_t i = 1; i <= CategoriesHolder::kNumLanguages; ++i)
    AddCategoryByTypeAndLang(type, i);
}

void CategoriesIndex::AddAllCategoriesInLang(int8_t lang)
{
  m_catHolder.ForEachTypeAndCategory([&](uint32_t type, Category const & cat)
                                     {
                                       for (auto const & name : cat.m_synonyms)
                                       {
                                         if (name.m_locale == lang)
                                           AddAllSubstrings(m_trie, name.m_name, type);
                                       }
                                     });
}

void CategoriesIndex::AddAllCategoriesAllLangs()
{
  m_catHolder.ForEachTypeAndCategory([this](uint32_t type, Category const & cat)
                                     {
                                       for (auto const & name : cat.m_synonyms)
                                         AddAllSubstrings(m_trie, name.m_name, type);
                                     });
}

void CategoriesIndex::GetCategories(string const & query, vector<Category> & result)
{
  vector<uint32_t> types;
  GetAssociatedTypes(query, types);
  my::SortUnique(types);
  m_catHolder.ForEachTypeAndCategory([&](uint32_t type, Category const & cat)
                                     {
                                       if (binary_search(types.begin(), types.end(), type))
                                         result.push_back(cat);
                                     });
}

void CategoriesIndex::GetAssociatedTypes(string const & query, vector<uint32_t> & result)
{
  set<uint32_t> types;
  auto fn = [&](string const & s, uint32_t type)
  {
    types.insert(type);
  };
  m_trie.ForEachInSubtree(query, fn);
  result.insert(result.end(), types.begin(), types.end());
}
}  // namespace indexer
