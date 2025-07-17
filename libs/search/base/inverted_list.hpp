#pragma once

#include "base/assert.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace search_base
{
// This class is supposed to be used in inverted index to store list
// of document ids.
template <typename Id>
class InvertedList
{
public:
  using value_type = Id;
  using Value = Id;

  bool Add(Id const & id)
  {
    auto it = std::lower_bound(m_ids.begin(), m_ids.end(), id);
    if (it != m_ids.end() && *it == id)
      return false;
    m_ids.insert(it, id);
    return true;
  }

  bool Erase(Id const & id)
  {
    auto it = std::lower_bound(m_ids.begin(), m_ids.end(), id);
    if (it == m_ids.end() || *it != id)
      return false;
    m_ids.erase(it);
    return true;
  }

  template <typename ToDo>
  void ForEach(ToDo && toDo) const
  {
    for (auto const & id : m_ids)
      toDo(id);
  }

  size_t Size() const { return m_ids.size(); }

  bool Empty() const { return Size() == 0; }

  void Clear() { m_ids.clear(); }

  void Swap(InvertedList & rhs) { m_ids.swap(rhs.m_ids); }

private:
  std::vector<Id> m_ids;
};
}  // namespace search_base
