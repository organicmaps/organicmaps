#pragma once
#include "../base/base.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"
#include "../std/algorithm.hpp"

class Reader;

namespace search
{

struct Category
{
  /// Classificator types
  vector<uint32_t> m_types;

  struct Name
  {
    string m_name;
    int8_t m_lang;
    uint8_t m_prefixLengthToSuggest;
  };

  /// <language, synonym>
  vector<Name> m_synonyms;
};

class CategoriesHolder
{
  typedef vector<Category> ContainerT;
  ContainerT m_categories;

public:
  typedef ContainerT::const_iterator const_iterator;

  CategoriesHolder();
  explicit CategoriesHolder(Reader const & reader);

  /// @return number of loaded categories or 0 if something goes wrong
  size_t LoadFromStream(string const & buffer);

  template <class ToDo>
  void ForEachCategory(ToDo toDo) const
  {
    for_each(m_categories.begin(), m_categories.end(), toDo);
  }

  const_iterator begin() const { return m_categories.begin(); }
  const_iterator end() const { return m_categories.end(); }

  void swap(CategoriesHolder & o);
};

inline void swap(CategoriesHolder & a, CategoriesHolder & b)
{
  return a.swap(b);
}

}
