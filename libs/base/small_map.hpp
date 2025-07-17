#pragma once

#include "assert.hpp"

#include <vector>

namespace base
{

/// Consider using as a replacement of unordered_map (map) when:
/// - very small amount of elements (<8)
template <class Key, class Value> class SmallMapBase
{
public:
  using ValueType = std::pair<Key, Value>;

  SmallMapBase() = default;
  SmallMapBase(std::initializer_list<ValueType> init) : m_map(std::move(init)) {}
  template <class Iter> SmallMapBase(Iter beg, Iter end) : m_map(beg, end) {}

  bool operator==(SmallMapBase const & rhs) const { return m_map == rhs.m_map; }

  void Reserve(size_t count) { m_map.reserve(count); }
  void Insert(Key k, Value v) { m_map.emplace_back(std::move(k), std::move(v)); }

  Value const * Find(Key const & k) const
  {
    for (auto const & e : m_map)
    {
      if (e.first == k)
        return &e.second;
    }
    return nullptr;
  }

  size_t size() const { return m_map.size(); }

protected:
  /// @todo buffer_vector is not suitable now, because Key/Value is not default constructible.
  std::vector<ValueType> m_map;
};

/// Consider using as a replacement of unordered_map (map) when:
/// - initialize and don't modify
/// - relatively small amount of elements (8-128)
template <class Key, class Value> class SmallMap : public SmallMapBase<Key, Value>
{
  using BaseT = SmallMapBase<Key, Value>;

public:
  using ValueType = typename BaseT::ValueType;

  SmallMap() = default;
  SmallMap(std::initializer_list<ValueType> init)
    : BaseT(std::move(init))
  {
    FinishBuilding();
  }
  template <class Iter> SmallMap(Iter beg, Iter end)
    : BaseT(beg, end)
  {
    FinishBuilding();
  }

  void FinishBuilding()
  {
    auto & theMap = this->m_map;
    std::sort(theMap.begin(), theMap.end(), [](ValueType const & l, ValueType const & r)
    {
      return l.first < r.first;
    });
  }

  Value const * Find(Key const & k) const
  {
    auto const & theMap = this->m_map;
    auto const it = std::lower_bound(theMap.cbegin(), theMap.cend(), k,
                                     [](ValueType const & l, Key const & r)
    {
      return l.first < r;
    });

    if (it != theMap.cend() && it->first == k)
      return &(it->second);
    return nullptr;
  }

  void Replace(Key const & k, Value v)
  {
    auto & theMap = this->m_map;
    auto it = std::lower_bound(theMap.begin(), theMap.end(), k,
                               [](ValueType const & l, Key const & r)
    {
      return l.first < r;
    });

    ASSERT(it != theMap.end() && it->first == k, ());
    it->second = std::move(v);
  }

  Value const & Get(Key const & k) const
  {
    Value const * v = Find(k);
    ASSERT(v, ());
    return *v;
  }
};

} // namespace base
