#pragma once

#include "categories_holder.hpp"

#include "base/mem_trie.hpp"

#include <string>
#include <vector>

namespace indexer
{
// Indexes synonym tokens from a CategoriesHolder for substring searches, such as the editor's
// category picker. Returned types are classifier types.
class CategoriesIndex
{
  DISALLOW_COPY(CategoriesIndex);

public:
  using Category = CategoriesHolder::Category;

  CategoriesIndex() : m_catHolder(&GetDefaultCategories()) {}

  explicit CategoriesIndex(CategoriesHolder const & catHolder) : m_catHolder(&catHolder) {}

  CategoriesIndex(CategoriesIndex &&) = default;
  CategoriesIndex & operator=(CategoriesIndex &&) = default;

  // Indexes synonyms for |type| in category locale |lang|. Locale codes are defined in categories_holder.hpp.
  void AddCategoryByTypeAndLang(uint32_t type, int8_t lang);

  // Indexes synonyms for |type| in all languages.
  void AddCategoryByTypeAllLangs(uint32_t type);

  // Indexes synonyms from every holder record in category locale |lang|.
  void AddAllCategoriesInLang(int8_t lang);

  // Indexes synonyms from every holder record in all languages.
  void AddAllCategoriesInAllLangs();

  // Appends holder records for matched types, including synonyms in languages not added to this index.
  // Beware weird results when query is a malformed UTF-8 string.
  void GetCategories(std::string const & query, std::vector<Category> & result) const;

  // Appends matching types in classifier order. Each normalized query token must match a substring
  // of an indexed synonym; tokens may match different synonyms. An empty query adds no types.
  // Beware weird results when query is a malformed UTF-8 string.
  void GetAssociatedTypes(std::string const & query, std::vector<uint32_t> & result) const;

#ifdef DEBUG
  inline size_t GetNumTrieNodes() const { return m_trie.GetNumNodes(); }
#endif

private:
  // The holder must outlive this index. A pointer allows move assignment without copying the holder.
  CategoriesHolder const * m_catHolder = nullptr;
  base::MemTrie<std::string, base::VectorValues<uint32_t>> m_trie;
};
}  // namespace indexer
