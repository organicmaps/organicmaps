#pragma once

#include "indexer/classificator.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/cstdint.hpp"
#include "std/map.hpp"

namespace search
{
class CategoriesSet
{
public:
  CategoriesSet() : m_classificator(classif()) {}

  void Add(uint32_t type)
  {
    uint32_t const index = m_classificator.GetIndexForType(type);
    m_categories[FeatureTypeToString(index)] = type;
  }

  template <typename TFn>
  void ForEach(TFn && fn) const
  {
    for (auto const & p : m_categories)
      fn(p.first, p.second);
  }

  bool HasKey(strings::UniString const & key) const
  {
    return m_categories.count(key) != 0;
  }

private:
  Classificator const & m_classificator;
  map<strings::UniString, uint32_t> m_categories;

  DISALLOW_COPY_AND_MOVE(CategoriesSet);
};
}  // namespace search
