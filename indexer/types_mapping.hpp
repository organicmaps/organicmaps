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

  static constexpr uint32_t INVALID_TYPE = 0;
  /// @return INVALID_TYPE If \a ind is out of bounds.
  uint32_t GetType(uint32_t ind) const { return ind < m_types.size() ? m_types[ind] : INVALID_TYPE; }

  uint32_t GetIndex(uint32_t t) const;

  /// For Debug purposes only.
  bool HasIndex(uint32_t t) const { return (m_map.find(t) != m_map.end()); }

private:
  using Map = std::map<uint32_t, uint32_t>;
  void Add(uint32_t ind, uint32_t type, bool isMainTypeDescription);

  std::vector<uint32_t> m_types;
  Map m_map;
};
