#pragma once

#include "indexer/classificator.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/macros.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/cstdint.hpp"
#include "std/vector.hpp"

namespace search
{
struct Category
{
  Category(strings::UniString const & key, uint32_t type) : m_key(key), m_type(type) {}

  bool operator==(Category const & rhs) const { return m_key == rhs.m_key && m_type == rhs.m_type; }

  bool operator<(Category const & rhs) const
  {
    if (m_key != rhs.m_key)
      return m_key < rhs.m_key;
    return m_type < rhs.m_type;
  }

  strings::UniString m_key;
  uint32_t m_type = 0;
};

class CategoriesSet
{
public:
  template <typename TTypesList>
  CategoriesSet(TTypesList const & typesList)
  {
    auto const & classificator = classif();

    auto addCategory = [&](uint32_t type) {
      uint32_t const index = classificator.GetIndexForType(type);
      m_categories.emplace_back(FeatureTypeToString(index), type);
    };

    typesList.ForEachType(addCategory);

    my::SortUnique(m_categories);
  }

  template <typename TFn>
  void ForEach(TFn && fn) const
  {
    for_each(m_categories.cbegin(), m_categories.cend(), forward<TFn>(fn));
  }

  bool HasKey(strings::UniString const & key) const
  {
    auto const it =
        lower_bound(m_categories.cbegin(), m_categories.cend(), Category(key, 0 /* type */));
    return it != m_categories.cend() && it->m_key == key;
  }

private:
  vector<Category> m_categories;

  DISALLOW_COPY_AND_MOVE(CategoriesSet);
};
}  // namespace search
