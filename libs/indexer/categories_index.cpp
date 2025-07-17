#include "categories_index.hpp"
#include "search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <set>

namespace indexer
{
using namespace std;

namespace
{
void AddAllNonemptySubstrings(base::MemTrie<std::string, base::VectorValues<uint32_t>> & trie,
                              std::string const & s, uint32_t value)
{
  ASSERT(!s.empty(), ());
  for (size_t i = 0; i < s.length(); ++i)
  {
    std::string t;
    for (size_t j = i; j < s.length(); ++j)
    {
      t.push_back(s[j]);
      trie.Add(t, value);
    }
  }
}

template <typename TF>
void ForEachToken(std::string const & s, TF && fn)
{
  search::ForEachNormalizedToken(s, [&fn](strings::UniString const & token)
  {
    fn(strings::ToUtf8(token));
  });
}

void TokenizeAndAddAllSubstrings(base::MemTrie<std::string, base::VectorValues<uint32_t>> & trie,
                                 std::string const & s, uint32_t value)
{
  auto fn = [&](std::string const & token)
  {
    AddAllNonemptySubstrings(trie, token, value);
  };
  ForEachToken(s, fn);
}
}  // namespace

void CategoriesIndex::AddCategoryByTypeAndLang(uint32_t type, int8_t lang)
{
  ASSERT(lang >= 1 && static_cast<size_t>(lang) <= CategoriesHolder::kLocaleMapping.size(),
         ("Invalid lang code:", lang));
  m_catHolder->ForEachNameByType(type, [&](Category::Name const & name)
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
  m_catHolder->ForEachTypeAndCategory([&](uint32_t type, Category const & cat)
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
  m_catHolder->ForEachTypeAndCategory([this](uint32_t type, Category const & cat)
                                      {
                                        for (auto const & name : cat.m_synonyms)
                                          TokenizeAndAddAllSubstrings(m_trie, name.m_name, type);
                                      });
}

void CategoriesIndex::GetCategories(std::string const & query, std::vector<Category> & result) const
{
  std::vector<uint32_t> types;
  GetAssociatedTypes(query, types);
  base::SortUnique(types);
  m_catHolder->ForEachTypeAndCategory([&](uint32_t type, Category const & cat)
                                      {
                                        if (binary_search(types.begin(), types.end(), type))
                                          result.push_back(cat);
                                      });
}

void CategoriesIndex::GetAssociatedTypes(std::string const & query, std::vector<uint32_t> & result) const
{
  bool first = true;
  std::set<uint32_t> intersection;
  auto processToken = [&](std::string const & token)
  {
    std::set<uint32_t> types;
    auto fn = [&](std::string const &, uint32_t type)
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
      std::set<uint32_t> tmp;
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
