#pragma once

#include "../std/vector.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"
#include "../std/istream.hpp"

namespace search
{

struct Category
{
  /// Classificator types
  vector<uint32_t> m_types;
  /// <language, synonym>
  vector<pair<signed char, string> > m_synonyms;
};

class CategoriesHolder
{
  typedef vector<Category> ContainerT;
  ContainerT m_categories;

public:
  /// @return number of loaded categories or 0 if something goes wrong
  size_t LoadFromStream(istream & stream);

  template <class T>
  void ForEachCategory(T & f) const
  {
    for (ContainerT::const_iterator it = m_categories.begin(); it != m_categories.end(); ++it)
      f(*it);
  }
};

}
