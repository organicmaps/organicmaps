#pragma once
#include "base/assert.hpp"
#include "base/base.hpp"
#include "std/sstream.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"

namespace m2
{
template <int kDepthLevels = 31>
class CellId
{
public:
  // Use enum to avoid linker errors.
  // Can't realize why static const or constexpr is invalid here ...
  enum
  {
    DEPTH_LEVELS = kDepthLevels
  };
  static uint8_t const MAX_CHILDREN = 4;
  static uint32_t const MAX_COORD = 1U << DEPTH_LEVELS;

  CellId() : m_Bits(0), m_Level(0) { ASSERT(IsValid(), ()); }
  explicit CellId(string const & s) { *this = FromString(s); }

  static CellId Root() { return CellId(0, 0); }
  static CellId FromBitsAndLevel(uint64_t bits, int level) { return CellId(bits, level); }
  static size_t TotalCellsOnLevel(size_t level) { return 1 << (2 * level); }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Simple getters

  int Level() const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    return m_Level;
  }

  CellId Parent() const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    ASSERT_GREATER(m_Level, 0, ());
    return CellId(m_Bits >> 2, m_Level - 1);
  }

  CellId AncestorAtLevel(int level) const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    ASSERT_GREATER_OR_EQUAL(m_Level, level, ());
    return CellId(m_Bits >> ((m_Level - level) << 1), level);
  }

  CellId Child(int8_t c) const
  {
    ASSERT(c >= 0 && c < 4, (c, m_Bits, m_Level));
    ASSERT(IsValid(), (m_Bits, m_Level));
    ASSERT_LESS(m_Level, DEPTH_LEVELS - 1, ());
    return CellId((m_Bits << 2) | c, m_Level + 1);
  }

  char WhichChildOfParent() const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    ASSERT_GREATER(m_Level, 0, ());
    return m_Bits & 3;
  }

  uint64_t SubTreeSize(int depth) const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    ASSERT(m_Level < depth && depth <= DEPTH_LEVELS, (m_Bits, m_Level, depth));
    return TreeSizeForDepth(depth - m_Level);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Operators

  bool operator==(CellId const & cellId) const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    ASSERT(cellId.IsValid(), (cellId.m_Bits, cellId.m_Level));
    return m_Bits == cellId.m_Bits && m_Level == cellId.m_Level;
  }

  bool operator!=(CellId const & cellId) const { return !(*this == cellId); }
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Conversion to/from string

  string ToString() const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    string result(m_Level, '0');
    uint64_t bits = m_Bits;
    for (int i = 0; i < m_Level; ++i, bits >>= 2)
      result[m_Level - 1 - i] += (bits & 3);
    ASSERT_EQUAL(*this, FromString(result), (m_Bits, m_Level, result));
    return result;
  }

  // Is string @s a valid CellId representation?
  // Note that empty string is a valid CellId.
  static bool IsCellId(string const & s)
  {
    size_t const length = s.size();
    if (length >= DEPTH_LEVELS)
      return false;
    for (size_t i = 0; i < length; ++i)
    {
      if (s[i] < '0' || s[i] > '3')
        return false;
    }
    return true;
  }

  static CellId FromString(string const & s)
  {
    ASSERT(IsCellId(s), (s));
    uint64_t bits = 0;
    size_t const level = s.size();
    ASSERT_LESS(level, static_cast<size_t>(DEPTH_LEVELS), (s));
    for (size_t i = 0; i < level; ++i)
    {
      ASSERT((s[i] >= '0') && (s[i] <= '3'), (s, i));
      bits = (bits << 2) | static_cast<uint64_t>(s[i] - '0');
    }
    return CellId(bits, static_cast<int>(level));
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Conversion to/from point

  // Cell area width and height.
  // Should be 1 for the bottom level cell.
  uint32_t Radius() const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    return 1 << (DEPTH_LEVELS - 1 - m_Level);
  }

  pair<uint32_t, uint32_t> XY() const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    uint32_t offset = 1 << (DEPTH_LEVELS - 1 - m_Level);
    pair<uint32_t, uint32_t> xy(offset, offset);
    uint64_t bits = m_Bits;
    while (bits > 0)
    {
      offset <<= 1;
      if (bits & 1)
        xy.first += offset;
      if (bits & 2)
        xy.second += offset;
      bits >>= 2;
    }
    ASSERT_EQUAL(*this, FromXY(xy.first, xy.second, m_Level), ());
    return xy;
  }

  static CellId FromXY(uint32_t x, uint32_t y, int level)
  {
    ASSERT_LESS(level, static_cast<int>(DEPTH_LEVELS), (x, y, level));
    // Since MAX_COORD == 1 << DEPTH_LEVELS, if x|y == MAX_COORD, they should be decremented.
    if (x >= MAX_COORD)
    {
      ASSERT_EQUAL(x, static_cast<uint32_t>(MAX_COORD), (x, y, level));
      x = MAX_COORD - 1;
    }
    if (y >= MAX_COORD)
    {
      ASSERT_EQUAL(y, static_cast<uint32_t>(MAX_COORD), (x, y, level));
      y = MAX_COORD - 1;
    }
    x >>= DEPTH_LEVELS - level;
    y >>= DEPTH_LEVELS - level;
    // This operation is called "perfect shuffle". Optimized bit trick can be used here.
    uint64_t bits = 0;
    for (int i = 0; i < level; ++i)
      bits |= ((uint64_t((y >> i) & 1) << (2 * i + 1)) | (uint64_t((x >> i) & 1) << (2 * i)));
    return CellId(bits, level);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Ordering

  struct LessLevelOrder
  {
    bool operator()(CellId<DEPTH_LEVELS> const & id1, CellId<DEPTH_LEVELS> const & id2) const
    {
      if (id1.m_Level != id2.m_Level)
        return id1.m_Level < id2.m_Level;
      if (id1.m_Bits != id2.m_Bits)
        return id1.m_Bits < id2.m_Bits;
      return false;
    }
  };

  struct GreaterLevelOrder
  {
    bool operator()(CellId<DEPTH_LEVELS> const & id1, CellId<DEPTH_LEVELS> const & id2) const
    {
      if (id1.m_Level != id2.m_Level)
        return id1.m_Level > id2.m_Level;
      if (id1.m_Bits != id2.m_Bits)
        return id1.m_Bits > id2.m_Bits;
      return false;
    }
  };

  struct LessPreOrder
  {
    bool operator()(CellId<DEPTH_LEVELS> const & id1, CellId<DEPTH_LEVELS> const & id2) const
    {
      int64_t const n1 = id1.ToInt64LevelZOrder(DEPTH_LEVELS);
      int64_t const n2 = id2.ToInt64LevelZOrder(DEPTH_LEVELS);
      ASSERT_EQUAL(n1 < n2, id1.ToString() < id2.ToString(), (id1, id2));
      return n1 < n2;
    }
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Numbering

  // Default ToInt64().
  int64_t ToInt64(int depth) const { return ToInt64LevelZOrder(depth); }
  // Default FromInt64().
  static CellId FromInt64(int64_t v, int depth) { return FromInt64LevelZOrder(v, depth); }
  // Level order, numbering by Z-curve.
  int64_t ToInt64LevelZOrder(int depth) const
  {
    ASSERT(0 < depth && depth <= DEPTH_LEVELS, (m_Bits, m_Level, depth));
    ASSERT(IsValid(), (m_Bits, m_Level));

    if (m_Level >= depth)
      return AncestorAtLevel(depth - 1).ToInt64(depth);
    else
    {
      uint64_t bits = m_Bits, res = 0;
      for (int i = 0; i <= m_Level; ++i, bits >>= 2)
        res += bits + 1;
      bits = m_Bits;
      for (int i = m_Level + 1; i < depth; ++i)
      {
        bits <<= 2;
        res += bits;
      }
      ASSERT_GREATER(res, 0, (m_Bits, m_Level));
      ASSERT_LESS_OR_EQUAL(res, TreeSizeForDepth(depth), (m_Bits, m_Level));
      return static_cast<int64_t>(res);
    }
  }

  // Level order, numbering by Z-curve.
  static CellId FromInt64LevelZOrder(int64_t v, int depth)
  {
    ASSERT_GREATER(v, 0, ());
    ASSERT(0 < depth && depth <= DEPTH_LEVELS, (v, depth));
    ASSERT_LESS_OR_EQUAL(v, TreeSizeForDepth(depth), ());
    uint64_t bits = 0;
    int level = 0;
    --v;
    while (v > 0)
    {
      bits <<= 2;
      ++level;
      uint64_t subTreeSize = TreeSizeForDepth(depth - level);
      for (--v; v >= subTreeSize; v -= subTreeSize)
        ++bits;
    }
    return CellId(bits, level);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////
private:
  static uint64_t TreeSizeForDepth(int depth)
  {
    ASSERT(0 < depth && depth <= DEPTH_LEVELS, (depth));
    return ((1ULL << 2 * depth) - 1) / 3ULL;
  }

  CellId(uint64_t bits, int level) : m_Bits(bits), m_Level(level)
  {
    ASSERT_LESS(level, DEPTH_LEVELS, (bits, level));
    ASSERT_LESS(bits, 1ULL << m_Level * 2, (bits, m_Level));
    ASSERT(IsValid(), (m_Bits, m_Level));
  }

  bool IsValid() const
  {
    if (m_Level < 0 || m_Level >= DEPTH_LEVELS)
      return false;
    if (m_Bits >= (1ULL << m_Level * 2))
      return false;
    return true;
  }

  uint64_t m_Bits;
  int m_Level;
};

template <int DEPTH_LEVELS>
string DebugPrint(CellId<DEPTH_LEVELS> const & id)
{
  ostringstream out;
  out << "CellId<" << DEPTH_LEVELS << ">(\"" << id.ToString().c_str() << "\")";
  return out.str();
}
}
