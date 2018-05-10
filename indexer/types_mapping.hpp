#pragma once
#include "base/assert.hpp"

#include "std/vector.hpp"
#include "std/map.hpp"
#include "std/iostream.hpp"


class IndexAndTypeMapping
{
  vector<uint32_t> m_types;

  typedef map<uint32_t, uint32_t> MapT;
  MapT m_map;

  void Add(uint32_t ind, uint32_t type);

public:
  void Clear();
  void Load(istream & s);

  // Throws std::out_of_range exception.
  uint32_t GetType(uint32_t ind) const
  {
    ASSERT_LESS ( ind, m_types.size(), () );
    return m_types.at(ind);
  }

  uint32_t GetIndex(uint32_t t) const;

  /// For Debug purposes only.
  bool HasIndex(uint32_t t) const { return (m_map.find(t) != m_map.end()); }
};
