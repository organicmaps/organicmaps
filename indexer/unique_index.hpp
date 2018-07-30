#pragma once

#include "base/base.hpp"

#include <unordered_set>
#include <vector>

class CheckUniqueIndexes
{
  std::unordered_set<uint32_t> m_s;
  std::vector<bool> m_v;
  bool m_useBits;

  /// Add index to the set.
  /// @return true If index was absent.
  bool Add(uint32_t index)
  {
    if (!m_useBits)
      return m_s.insert(index).second;

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
    if (!m_useBits)
      return (m_s.erase(index) > 0);

    if (m_v.size() > index)
    {
      bool const ret = m_v[index];
      m_v[index] = false;
      return ret;
    }
    else
      return false;
  }

public:
  explicit CheckUniqueIndexes(bool useBits) : m_useBits(useBits) {}

  bool operator()(uint32_t index)
  {
    return Add(index);
  }
};
