#pragma once

#include "base/geo_object_id.hpp"

#include <vector>

namespace geocoder
{
// A data structure to perform the beam search with.
// Maintains a list of (Key, Value) pairs sorted in the decreasing
// order of Values.
class Beam
{
public:
  using Key = base::GeoObjectId;
  using Value = double;

  struct Entry
  {
    Key m_key;
    Value m_value;

    Entry(Key const & key, Value value) : m_key(key), m_value(value) {}

    bool operator<(Entry const & rhs) const { return m_value > rhs.m_value; }
  };

  explicit Beam(size_t capacity);

  // O(log(n) + n) for |n| entries.
  // O(|m_capacity|) in the worst case.
  void Add(Key const & key, Value value);

  std::vector<Entry> const & GetEntries() const { return m_entries; }

private:
  size_t m_capacity;
  std::vector<Entry> m_entries;
};
}  // namespace geocoder
