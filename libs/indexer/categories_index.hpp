#pragma once

#include "categories_holder.hpp"

#include "base/mem_trie.hpp"

#include <string>
#include <vector>

namespace indexer
{
// This class is used to simplify searches of categories by
// synonyms to their names (in various languages).
// An example usage is helping a user who is trying to add
// a new feature with our editor.
// Synonyms come from the holder; callers can also index display names without modifying its records.
// All types returned are those from classificator.
class CategoriesIndex
{
  DISALLOW_COPY(CategoriesIndex);

public:
  using Category = CategoriesHolder::Category;

  CategoriesIndex() : m_catHolder(&GetDefaultCategories()) {}

  explicit CategoriesIndex(CategoriesHolder const & catHolder) : m_catHolder(&catHolder) {}

  CategoriesIndex(CategoriesIndex &&) = default;
  CategoriesIndex & operator=(CategoriesIndex &&) = default;

  // Adds all categories that match |type|. Only synonyms
  // in language |lang| are added. See indexer/categories_holder.hpp
  // for language enumeration.
  void AddCategoryByTypeAndLang(uint32_t type, int8_t lang);

  // Indexes an additional name for |type| using the same token normalization as category synonyms.
  void AddCategoryName(uint32_t type, std::string const & name);

  // Adds all categories that match |type|. All known synonyms
  // are added.
  void AddCategoryByTypeAllLangs(uint32_t type);

  // Adds all categories from the holder. Only
  // names in language |lang| are added.
  void AddAllCategoriesInLang(int8_t lang);

  // Adds all categories from the holder.
  void AddAllCategoriesInAllLangs();

  // Returns holder records for the matched types, including synonyms in languages not added to this index.
  // Additional names indexed with AddCategoryName() do not alter those records.
  // Beware weird results when query is a malformed UTF-8 string.
  void GetCategories(std::string const & query, std::vector<Category> & result) const;

  // Returns all types that match to categories that have |query| as substring.
  // Beware weird results when query is a malformed UTF-8 string.
  // Note: no types are returned if the query is empty.
  void GetAssociatedTypes(std::string const & query, std::vector<uint32_t> & result) const;

#ifdef DEBUG
  inline size_t GetNumTrieNodes() const { return m_trie.GetNumNodes(); }
#endif

private:
  // A non-owning pointer keeps the index move-assignable. The holder must outlive the index.
  CategoriesHolder const * m_catHolder = nullptr;
  base::MemTrie<std::string, base::VectorValues<uint32_t>> m_trie;
};
}  // namespace indexer
