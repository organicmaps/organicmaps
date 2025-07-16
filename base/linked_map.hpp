#pragma once

#include "base/assert.hpp"

#include <list>
#include <unordered_map>

namespace base
{
template <typename Key, typename Value, template <typename...> typename Map = std::unordered_map>
class LinkedMap
{
public:
  using KeyType = Key;
  using ValueType = Value;
  using ListType = std::list<std::pair<KeyType, ValueType>>;
  using MapType = Map<Key, typename ListType::iterator>;

  template <typename T>
  bool Emplace(KeyType const & key, T && value)
  {
    if (m_map.find(key) != m_map.cend())
      return false;

    m_list.emplace_back(key, std::forward<T>(value));
    m_map.emplace(key, std::prev(m_list.end()));
    return true;
  }

  void PopFront()
  {
    CHECK(!m_map.empty(), ());
    m_map.erase(m_list.front().first);
    m_list.pop_front();
  }

  bool Erase(KeyType const & key)
  {
    auto const it = m_map.find(key);
    if (it == m_map.cend())
      return false;

    m_list.erase(it->second);
    m_map.erase(it);
    return true;
  }

  bool Contains(KeyType const & key) const { return m_map.find(key) != m_map.cend(); }

  ValueType const & Get(KeyType const & key) const
  {
    auto const it = m_map.find(key);
    CHECK(it != m_map.cend(), ());

    return it->second->second;
  }

  ValueType & Front() { return m_list.front().second; }

  ValueType const & Front() const { return m_list.front().second; }

  void Swap(LinkedMap & linkedMap)
  {
    m_map.swap(linkedMap.m_map);
    m_list.swap(linkedMap.m_list);
  }

  size_t Size() const { return m_list.size(); }

  bool IsEmpty() const { return m_list.empty(); }

private:
  ListType m_list;
  MapType m_map;
};
}  // namespace base
