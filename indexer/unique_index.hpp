#pragma once

#include "../base/base.hpp"

#include "../std/vector.hpp"


class CheckUniqueIndexes
{
  vector<bool> m_v;

public:
  /// Add index to the set.
  /// @return true If index was absent.
  bool Add(uint32_t ind)
  {
    if (m_v.size() <= ind)
      m_v.resize(ind + 1);
    bool const ret = !m_v[ind];
    m_v[ind] = true;
    return ret;
  }

  /// Remove index from the set.
  /// @return true If index was present.
  bool Remove(uint32_t ind)
  {
    if (m_v.size() > ind)
    {
      bool const ret = m_v[ind];
      m_v[ind] = false;
      return ret;
    }
    else
      return false;
  }

  bool operator()(uint32_t ind)
  {
    return Add(ind);
  }
};
