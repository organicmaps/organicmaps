#pragma once

#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/bits.hpp"

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

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

  CellId() : m_bits(0), m_level(0) { ASSERT(IsValid(), ()); }
  explicit CellId(std::string const & s) { *this = FromString(s); }

  static CellId Root() { return CellId(0, 0); }
  static CellId FromBitsAndLevel(uint64_t bits, int level) { return CellId(bits, level); }
  static size_t TotalCellsOnLevel(size_t level) { return 1 << (2 * level); }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Simple getters
  int Level() const
  {
    ASSERT(IsValid(), (m_bits, m_level));
    return m_level;
  }

  CellId Parent() const
  {
    ASSERT(IsValid(), (m_bits, m_level));
    ASSERT_GREATER(m_level, 0, ());
    return CellId(m_bits >> 2, m_level - 1);
  }

  CellId AncestorAtLevel(int level) const
  {
    ASSERT(IsValid(), (m_bits, m_level));
    ASSERT_GREATER_OR_EQUAL(m_level, level, ());
    return CellId(m_bits >> (2 * (m_level - level)), level);
  }

  CellId Child(int8_t c) const
  {
    ASSERT(c >= 0 && c < 4, (c, m_bits, m_level));
    ASSERT(IsValid(), (m_bits, m_level));
    ASSERT_LESS(m_level, DEPTH_LEVELS - 1, ());
    return CellId((m_bits << 2) | c, m_level + 1);
  }

  char WhichChildOfParent() const
  {
    ASSERT(IsValid(), (m_bits, m_level));
    ASSERT_GREATER(m_level, 0, ());
    return m_bits & 3;
  }

  uint64_t SubTreeSize(int depth) const
  {
    ASSERT(IsValid(), (m_bits, m_level));
    ASSERT(m_level < depth && depth <= DEPTH_LEVELS, (m_bits, m_level, depth));
    return TreeSizeForDepth(depth - m_level);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Operators
  bool operator==(CellId const & cellId) const
  {
    ASSERT(IsValid(), (m_bits, m_level));
    ASSERT(cellId.IsValid(), (cellId.m_bits, cellId.m_level));
    return m_bits == cellId.m_bits && m_level == cellId.m_level;
  }

  bool operator!=(CellId const & cellId) const { return !(*this == cellId); }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Conversion to/from string
  std::string ToString() const
  {
    ASSERT(IsValid(), (m_bits, m_level));
    std::string result(m_level, '0');
    uint64_t bits = m_bits;
    for (int i = 0; i < m_level; ++i, bits >>= 2)
      result[m_level - 1 - i] += (bits & 3);
    ASSERT_EQUAL(*this, FromString(result), (m_bits, m_level, result));
    return result;
  }

  // Tests whether the string |s| is a valid CellId representation.
  // Note that an empty string is a valid CellId.
  static bool IsCellId(std::string const & s)
  {
    size_t const length = s.size();
    if (length >= DEPTH_LEVELS)
      return false;
    for (size_t i = 0; i < length; ++i)
      if (s[i] < '0' || s[i] > '3')
        return false;
    return true;
  }

  static CellId FromString(std::string const & s)
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
    ASSERT(IsValid(), (m_bits, m_level));
    return 1 << (DEPTH_LEVELS - 1 - m_level);
  }

  std::pair<uint32_t, uint32_t> XY() const
  {
    ASSERT(IsValid(), (m_bits, m_level));
    std::pair<uint32_t, uint32_t> xy;
    bits::BitwiseSplit(m_bits, xy.first, xy.second);
    xy.first = 2 * xy.first + 1;
    xy.second = 2 * xy.second + 1;
    xy.first <<= DEPTH_LEVELS - 1 - m_level;
    xy.second <<= DEPTH_LEVELS - 1 - m_level;
    ASSERT_EQUAL(*this, FromXY(xy.first, xy.second, m_level), ());
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
    uint64_t const bits = bits::BitwiseMerge(x, y);
    return CellId(bits, level);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Ordering
  struct LessLevelOrder
  {
    bool operator()(CellId<DEPTH_LEVELS> const & id1, CellId<DEPTH_LEVELS> const & id2) const
    {
      if (id1.m_level != id2.m_level)
        return id1.m_level < id2.m_level;
      if (id1.m_bits != id2.m_bits)
        return id1.m_bits < id2.m_bits;
      return false;
    }
  };

  struct GreaterLevelOrder
  {
    bool operator()(CellId<DEPTH_LEVELS> const & id1, CellId<DEPTH_LEVELS> const & id2) const
    {
      if (id1.m_level != id2.m_level)
        return id1.m_level > id2.m_level;
      if (id1.m_bits != id2.m_bits)
        return id1.m_bits > id2.m_bits;
      return false;
    }
  };

  struct LessPreOrder
  {
    bool operator()(CellId<DEPTH_LEVELS> const & id1, CellId<DEPTH_LEVELS> const & id2) const
    {
      int64_t const n1 = id1.ToInt64ZOrder(DEPTH_LEVELS);
      int64_t const n2 = id2.ToInt64ZOrder(DEPTH_LEVELS);
      ASSERT_EQUAL(n1 < n2, id1.ToString() < id2.ToString(), (id1, id2));
      return n1 < n2;
    }
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Numbering

  // Default ToInt64().
  int64_t ToInt64(int depth) const { return ToInt64ZOrder(depth); }

  // Default FromInt64().
  static CellId FromInt64(int64_t v, int depth) { return FromInt64ZOrder(v, depth); }

  // Returns the 1-based number this cell would get in the Z-curve ordering (pre-order
  // traversal of the quadtree) of all the cells in the tree of depth |depth|.
  // The tree of one vertex is assumed to have depth 1.
  int64_t ToInt64ZOrder(int depth) const
  {
    ASSERT(0 < depth && depth <= DEPTH_LEVELS, (m_bits, m_level, depth));
    ASSERT(IsValid(), (m_bits, m_level));

    if (m_level >= depth)
      return AncestorAtLevel(depth - 1).ToInt64ZOrder(depth);

    uint64_t bits = m_bits;
    uint64_t res = 0;
    // When counting, group the nodes by their level.
    // All nodes to the left of the current one at its level
    // and, for every ancestor, all nodes to the left of the ancestor
    // at the ancestor's level will show up earlier during the traversal.
    // All ancestors are visited before the current node, so add +1 for them.
    // The result is 1-based, so add +1 for the node itself too.
    for (int i = 0; i <= m_level; ++i)
    {
      res += bits + 1;
      bits >>= 2;
    }

    // By the same reasoning, if the children of every node are ordered
    // left to right, then all nodes at deeper levels that
    // are strictly to the left will be visited before the current one.
    bits = m_bits;
    for (int i = m_level + 1; i < depth; ++i)
    {
      bits <<= 2;
      res += bits;
    }

    ASSERT_GREATER(res, 0, (m_bits, m_level));
    ASSERT_LESS_OR_EQUAL(res, TreeSizeForDepth(depth), (m_bits, m_level));
    return static_cast<int64_t>(res);
  }

  // Returns the CellId with the 1-based number |v| in the Z-curve ordering (pre-order
  // traversal of the quadtree) of all the cells in the tree of depth |depth|.
  // The tree of one vertex is assumed to have depth 1.
  static CellId FromInt64ZOrder(int64_t v, int depth)
  {
    ASSERT_GREATER(v, 0, ());
    ASSERT(0 < depth && depth <= DEPTH_LEVELS, (v, depth));
    ASSERT_LESS_OR_EQUAL(static_cast<uint64_t>(v), TreeSizeForDepth(depth), ());
    uint64_t bits = 0;
    int level = 0;
    while (v > 1)
    {
      bits <<= 2;
      ++level;
      uint64_t const subtreeSize = TreeSizeForDepth(depth - level);
      for (--v; static_cast<uint64_t>(v) > subtreeSize; v -= subtreeSize)
        ++bits;
    }
    return CellId(bits, level);
  }

private:
  // Returns the total number of nodes in the tree of depth |depth|.
  // The tree of one vertex is assumed to have depth 1.
  // The first few values are 1, 5, 21, 85.
  static uint64_t TreeSizeForDepth(int depth)
  {
    ASSERT(0 < depth && depth <= DEPTH_LEVELS, (depth));
    return ((1ULL << 2 * depth) - 1) / 3ULL;
  }

  CellId(uint64_t bits, int level) : m_bits(bits), m_level(level)
  {
    ASSERT_LESS(level, DEPTH_LEVELS, (bits, level));
    ASSERT_LESS(bits, 1ULL << m_level * 2, (bits, m_level));
    ASSERT(IsValid(), (m_bits, m_level));
  }

  bool IsValid() const
  {
    if (m_level < 0 || m_level >= DEPTH_LEVELS)
      return false;
    if (m_bits >= (1ULL << m_level * 2))
      return false;
    return true;
  }

  uint64_t m_bits;
  int m_level;
};

template <int DEPTH_LEVELS>
std::string DebugPrint(CellId<DEPTH_LEVELS> const & id)
{
  std::ostringstream out;
  out << "CellId<" << DEPTH_LEVELS << ">(\"" << id.ToString().c_str() << "\")";
  return out.str();
}
}  // namespace m2
