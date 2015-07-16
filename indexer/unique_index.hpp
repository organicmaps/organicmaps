#pragma once

#include "../base/base.hpp"

#include "../std/unordered_set.hpp"
#include "../std/vector.hpp"


class CheckUniqueIndexes
{
  unordered_set<uint32_t> m_s;
  vector<bool> m_v;
  bool m_useBits;

  /// Add index to the set.
  /// @return true If index was absent.
  bool Add(uint32_t ind)
  {
    if (m_useBits)
    {
      if (m_v.size() <= ind)
        m_v.resize(ind + 1);
      bool const ret = !m_v[ind];
      m_v[ind] = true;
      return ret;
    }
    else
      return m_s.insert(ind).second;
  }

  /// Remove index from the set.
  /// @return true If index was present.
  bool Remove(uint32_t ind)
  {
    if (m_useBits)
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
    else
      return (m_s.erase(ind) > 0);
  }

public:
  explicit CheckUniqueIndexes(bool useBits) : m_useBits(useBits) {}

  bool operator()(uint32_t ind)
  {
    return Add(ind);
  }
};
