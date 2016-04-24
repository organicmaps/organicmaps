#include "categories_holder.hpp"

#include "base/mem_trie.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

namespace indexer
{
// This class is used to simplify searches of categories by
// synonyms to their names (in various languages).
// An example usage is helping a user who is trying to add
// a new feature with our editor.
// All category data is taken from data/categories.txt.
// All types returned are those from classificator.
class CategoriesIndex
{
public:
  using Category = CategoriesHolder::Category;

  CategoriesIndex() : m_catHolder(GetDefaultCategories()) {}

  CategoriesIndex(CategoriesHolder const & catHolder) : m_catHolder(catHolder) {}

  // Adds all categories that match |type|. Only synonyms
  // in language |lang| are added. See indexer/categories_holder.cpp
  // for language enumeration.
  void AddCategoryByTypeAndLang(uint32_t type, int8_t lang);

  // Adds all categories that match |type|. All known synonyms
  // are added.
  void AddCategoryByTypeAllLangs(uint32_t type);

  // Adds all categories from data/classificator.txt. Only
  // names in language |lang| are added.
  void AddAllCategoriesInLang(int8_t lang);

  // Adds all categories from data/classificator.txt.
  void AddAllCategoriesAllLangs();

  // Returns all categories that have |query| as a substring. Note
  // that all synonyms for a category are contained in a returned
  // value even if only one language was used when adding this
  // category's name to index.
  // Beware weird results when query is a malformed UTF-8 string.
  void GetCategories(string const & query, vector<Category> & result);

  // Returns all types that match to categories that have |query| as substring.
  // Beware weird results when query is a malformed UTF-8 string.
  void GetAssociatedTypes(string const & query, vector<uint32_t> & result);

#ifdef DEBUG
  int GetNumTrieNodes() { return m_trie.GetNumNodes(); }
#endif

private:
  CategoriesHolder const & m_catHolder;
  my::MemTrie<string, uint32_t> m_trie;
};
}  // namespace indexer
