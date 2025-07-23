#pragma once

#include <cstddef>
#include <unordered_map>

namespace base
{
// A bidirectional map to store a one-to-one correspondence between
// keys and values.
template <typename K, typename V, template <typename...> typename KToVMap = std::unordered_map,
          typename KToVHashOrComparator = std::hash<K>, template <typename...> typename VToKMap = std::unordered_map,
          typename VToKHashOrComparator = std::hash<V>>
class BidirectionalMap
{
public:
  BidirectionalMap() = default;

  size_t Size() const { return m_kToV.size(); }

  bool IsEmpty() const { return m_kToV.empty(); }

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

  bool RemoveKey(K const & k) { return Remove(k, m_kToV, m_vToK); }

  bool RemoveValue(V const & v) { return Remove(v, m_vToK, m_kToV); }

  void Swap(BidirectionalMap & other)
  {
    m_kToV.swap(other.m_kToV);
    m_vToK.swap(other.m_vToK);
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

protected:
  using Key = K;
  using Value = V;
  using KeysToValues = KToVMap<K, V, KToVHashOrComparator>;
  using ValuesToKeys = VToKMap<V, K, VToKHashOrComparator>;

  KeysToValues const & GetKeysToValues() const { return m_kToV; }

  ValuesToKeys const & GetValuesToKeys() const { return m_vToK; }

private:
  template <typename T, typename FirstMap, typename SecondMap>
  bool Remove(T const & t, FirstMap & firstMap, SecondMap & secondMap)
  {
    auto const firstIt = firstMap.find(t);
    if (firstIt == firstMap.cend())
      return false;

    secondMap.erase(firstIt->second);
    firstMap.erase(firstIt);
    return true;
  }

  KeysToValues m_kToV;
  ValuesToKeys m_vToK;
};
}  // namespace base
