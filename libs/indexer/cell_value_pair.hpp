#pragma once

#include "base/macros.hpp"

#include <cstdint>

namespace covering
{
template <typename Value>
class CellValuePair
{
public:
  using ValueType = Value;

  CellValuePair() = default;

  CellValuePair(uint64_t cell, Value value) : m_cellLo(UINT64_LO(cell)), m_cellHi(UINT64_HI(cell)), m_value(value) {}

  bool operator<(CellValuePair const & rhs) const
  {
    if (m_cellHi != rhs.m_cellHi)
      return m_cellHi < rhs.m_cellHi;
    if (m_cellLo != rhs.m_cellLo)
      return m_cellLo < rhs.m_cellLo;
    return m_value < rhs.m_value;
  }

  uint64_t GetCell() const { return UINT64_FROM_UINT32(m_cellHi, m_cellLo); }
  Value GetValue() const { return m_value; }

private:
  uint32_t m_cellLo = 0;
  uint32_t m_cellHi = 0;
  Value m_value = 0;
};

// For backward compatibility.
static_assert(sizeof(CellValuePair<uint32_t>) == 12);
static_assert(std::is_trivially_copyable<CellValuePair<uint32_t>>::value);
}  // namespace covering
