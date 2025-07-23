#pragma once

#include "base/assert.hpp"

#include <cstddef>
#include <functional>
#include <unordered_map>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-copy"
#endif
#include <boost/circular_buffer.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

template <class Key, class Value, class HashContainer = std::unordered_map<Key, Value>>
class FifoCache
{
  template <typename K, typename V>
  friend class FifoCacheTest;

public:
  using Loader = std::function<void(Key const & key, Value & value)>;

  /// \param capacity maximum size of the cache in number of items.
  /// \param loader Function which is called if it's necessary to load a new item for the cache.
  FifoCache(size_t capacity, Loader const & loader) : m_fifo(capacity), m_capacity(capacity), m_loader(loader) {}

  /// \brief Loads value, if it's necessary, by |key| with |m_loader|, puts it to cache and
  /// returns the reference to the value to |m_map|.
  Value const & GetValue(Key const & key)
  {
    auto const it = m_map.find(key);
    if (it != m_map.cend())
      return it->second;

    if (Size() >= m_capacity)
    {
      CHECK(!m_fifo.empty(), ());
      m_map.erase(m_fifo.back());
    }

    m_fifo.push_front(key);

    auto & v = m_map[key];
    m_loader(key, v);
    return v;
  }

private:
  size_t Size() const { return m_map.size(); }

  HashContainer m_map;
  boost::circular_buffer<Key> m_fifo;
  size_t m_capacity;
  Loader m_loader;
};
