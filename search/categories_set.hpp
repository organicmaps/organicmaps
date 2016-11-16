#pragma once

#include "indexer/classificator.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/cstdint.hpp"
#include "std/unordered_set.hpp"

namespace search
{
class CategoriesSet
{
public:
  CategoriesSet() : m_classificator(classif()) {}

  inline void Add(uint32_t type) { m_categories.insert(type); }

  template <typename Fn>
  void ForEach(Fn && fn) const
  {
    for_each(m_categories.begin(), m_categories.end(), forward<Fn>(fn));
  }

private:
  Classificator const & m_classificator;
  unordered_set<uint32_t> m_categories;

  DISALLOW_COPY_AND_MOVE(CategoriesSet);
};
}  // namespace search
