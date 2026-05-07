#pragma once

#include "base/macros.hpp"

#include <algorithm>
#include <unordered_set>

namespace search
{
class CategoriesSet
{
public:
  CategoriesSet() = default;

  void Add(uint32_t type)
  {
    ASSERT(type != 0, ());  // Classificator::INVALID_TYPE
    m_categories.insert(type);
  }

  template <typename Fn>
  void ForEach(Fn && fn) const
  {
    std::for_each(m_categories.begin(), m_categories.end(), std::forward<Fn>(fn));
  }

private:
  std::unordered_set<uint32_t> m_categories;

  DISALLOW_COPY_AND_MOVE(CategoriesSet);
};
}  // namespace search
