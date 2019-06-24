#pragma once

#include "base/exception.hpp"
#include "base/logging.hpp"

#include <cstddef>
#include <unordered_map>

namespace base
{
template <typename K, typename V>
class BidirectionalMap
{
public:
  DECLARE_EXCEPTION(Exception, RootException);
  DECLARE_EXCEPTION(NoKeyForValueException, Exception);
  DECLARE_EXCEPTION(NoValueForKeyException, Exception);

  BidirectionalMap() = default;

  size_t Size() const { return m_kToV.size(); }
  void Clear()
  {
    m_kToV.clear();
    m_vToK.clear();
  }

  void Add(K const & k, V const & v)
  {
    auto const resKV = m_kToV.emplace(k, v);
    if (!resKV.second)
    {
      LOG(LWARNING,
          ("Duplicate key in a BidirectionalMap:", k, "old value:", m_kToV.at(k), "new value:", v));
    }

    auto const resVK = m_vToK.emplace(v, k);
    if (!resVK.second)
    {
      LOG(LWARNING,
          ("Duplicate value in a BidirectionalMap:", v, "old key:", m_vToK.at(v), "new key:", k));
    }
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
    if (it == m_kToV.end())
      return false;
    value = it->second;
    return true;
  }

  bool GetKey(V const & value, K & key) const
  {
    auto const it = m_vToK.find(value);
    if (it == m_vToK.end())
      return false;
    key = it->second;
    return true;
  }

  V MustGetValue(K const & key) const
  {
    V result;
    if (!GetValue(key, result))
      MYTHROW(NoValueForKeyException, (key));
    return result;
  }

  K MustGetKey(V const & value) const
  {
    K result;
    if (!GetKey(value, result))
      MYTHROW(NoKeyForValueException, (value));
    return result;
  }

private:
  std::unordered_map<K, V> m_kToV;
  std::unordered_map<V, K> m_vToK;
};
}  // namespace base
