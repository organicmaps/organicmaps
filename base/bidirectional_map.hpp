#pragma once

#include <cstddef>
#include <unordered_map>

namespace base
{
// A bidirectional map to store a one-to-one correspondence between
// keys and values.
template <typename K, typename V>
class BidirectionalMap
{
public:
  BidirectionalMap() = default;

  size_t Size() const { return m_kToV.size(); }
  void Clear()
  {
    m_kToV.clear();
    m_vToK.clear();
  }

  bool Add(K const & k, V const & v)
  {
    if (m_kToV.find(k) != m_kToV.cend() || m_vToK.find(v) != m_vToK.cend())
      return false;

    m_kToV.emplace(k, v);
    m_vToK.emplace(v, k);
    return true;
  }

  template <typename Fn>
  void ForEachEntry(Fn && fn) const
  {
    for (auto const & e : m_kToV)
      fn(e.first, e.second);
  }

  bool GetValue(K const & key, V & value) const
  {
    auto const it = m_kToV.find(key);
    if (it == m_kToV.cend())
      return false;
    value = it->second;
    return true;
  }

  bool GetKey(V const & value, K & key) const
  {
    auto const it = m_vToK.find(value);
    if (it == m_vToK.cend())
      return false;
    key = it->second;
    return true;
  }

private:
  std::unordered_map<K, V> m_kToV;
  std::unordered_map<V, K> m_vToK;
};
}  // namespace base
