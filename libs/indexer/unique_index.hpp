#pragma once

#include "base/base.hpp"

#include <unordered_set>
#include <vector>

class CheckUniqueIndexes
{
public:
  bool operator()(uint32_t index) { return Add(index); }

private:
  /// Add index to the set.
  /// @return true If index was absent.
  bool Add(uint32_t index)
  {
    if (m_v.size() <= index)
      m_v.resize(index + 1);
    bool const ret = !m_v[index];
    m_v[index] = true;
    return ret;
  }

  /// Remove index from the set.
  /// @return true If index was present.
  bool Remove(uint32_t index)
  {
    if (m_v.size() > index)
    {
      bool const ret = m_v[index];
      m_v[index] = false;
      return ret;
    }
    else
      return false;
  }

  std::vector<bool> m_v;
};
