#pragma once

#include "base/assert.hpp"

#include <cstddef>
#include <functional>
#include <list>
#include <unordered_map>

template<class Key, class Value>
class FifoCache
{
  template <typename K, typename V> friend class FifoCacheTest;

public:
  using Loader = std::function<void(Key const & key, Value & value)>;

  /// \param capacity maximum size of the cache in number of items.
  /// \param loader Function which is called if it's necessary to load a new item for the cache.
  FifoCache(size_t capacity, Loader const & loader) : m_capacity(capacity), m_loader(loader) {}

  /// \brief Loads value, if it's necessary, by |key| with |m_loader|, puts it to cache and
  /// returns the reference to the value to |m_map|.
  Value const & GetValue(const Key & key)
  {
    auto const it = m_map.find(key);
    if (it != m_map.cend())
      return it->second;

    if (Size() >= m_capacity)
      Evict();

    m_list.push_front(key);

    auto & v = m_map[key];
    m_loader(key, v);
    return (m_map[key] = v);
  }

private:
  void Evict()
  {
    auto const i = --m_list.end();
    m_map.erase(*i);
    m_list.erase(i);
  }

  size_t Size() const { return m_map.size(); }

  std::unordered_map<Key, Value> m_map;
  // @TODO(bykoianko) Consider using a structure like boost/circular_buffer instead of list
  // but with handling overwriting shift. We need it to know to remove overwritten key from |m_map|.
  std::list<Key> m_list;
  size_t m_capacity;
  Loader m_loader;
};
