#pragma once
#include "base/assert.hpp"

#include <cstdint>
#include <iostream>
#include <map>
#include <vector>


class IndexAndTypeMapping
{
public:
  void Clear();
  void Load(std::istream & s);
  bool IsLoaded() const { return !m_types.empty(); }

  // Throws std::out_of_range exception.
  uint32_t GetType(uint32_t ind) const
  {
    ASSERT_LESS ( ind, m_types.size(), () );
    return m_types.at(ind);
  }

  uint32_t GetIndex(uint32_t t) const;

  /// For Debug purposes only.
  bool HasIndex(uint32_t t) const { return (m_map.find(t) != m_map.end()); }

private:
  using Map = std::map<uint32_t, uint32_t>;
  void Add(uint32_t ind, uint32_t type);

  std::vector<uint32_t> m_types;
  Map m_map;
};
