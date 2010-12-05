#pragma once
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../std/string.hpp"
#include "../std/sstream.hpp"
#include "../std/utility.hpp"


namespace m2
{

#pragma pack(push, 1)
template <int kDepthLevels> class CellId
{
public:
  enum { DEPTH_LEVELS = kDepthLevels };
  enum { MAX_COORD = 1 << DEPTH_LEVELS };

  explicit CellId(string const & s)
  {
    *this = FromString(s);
  }

  pair<int64_t, int> ToBitsAndLevel() const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    return pair<int64_t, int>(m_Bits, m_Level);
  }

  // Only some number of lowest bits are used.
  int64_t ToInt64() const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    int64_t bits = m_Bits, res = 0;
    for (int i = 0; i <= m_Level; ++i, bits >>= 2)
      res += bits + 1;
    bits = m_Bits;
    for (int i = m_Level + 1; i < DEPTH_LEVELS; ++i)
    {
      bits <<= 2;
      res += bits;
    }
    ASSERT_GREATER(res, 0, (m_Bits, m_Level));
    ASSERT_LESS_OR_EQUAL(res, SubTreeSize(DEPTH_LEVELS - 1), (m_Bits, m_Level));
    return res;
  }

  static CellId Root()
  {
    return CellId(0, 0);
  }

  static CellId FromBitsAndLevel(int64_t bits, int level)
  {
    return CellId(bits, level);
  }

  static CellId FromString(string const & s)
  {
    int64_t bits = 0;
    size_t const level = s.size();
    ASSERT_LESS ( level, static_cast<size_t>(DEPTH_LEVELS), (s) );
    for (size_t i = 0; i < level; ++i)
    {
      ASSERT((s[i] >= '0') && (s[i] <= '3'), (s, i));
      bits = (bits << 2) | (s[i] - '0');
    }
    return CellId(bits, static_cast<int>(level));
  }

  static CellId FromInt64(int64_t v)
  {
    ASSERT_GREATER(v, 0, ());
    ASSERT_LESS_OR_EQUAL(v, SubTreeSize(DEPTH_LEVELS - 1), ());
    int64_t bits = 0;
    int level = 0;
    --v;
    while (v > 0)
    {
      bits <<= 2;
      ++level;
      int64_t subTreeSize = SubTreeSize(DEPTH_LEVELS - 1 - level);
      for (--v; v >= subTreeSize; v -= subTreeSize)
        ++bits;
    }
    return CellId(bits, level);
  }

  string ToString() const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    string result(m_Level, '0');
    int64_t bits = m_Bits;
    for (int i = 0; i < m_Level; ++i, bits >>= 2)
      result[m_Level - 1 - i] += (bits & 3);
    ASSERT_EQUAL(*this, FromString(result), (m_Bits, m_Level, result));
    return result;
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

  CellId Child(char c) const
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

  inline bool operator == (CellId const & cellId) const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    ASSERT(cellId.IsValid(), (cellId.m_Bits, cellId.m_Level));
    return m_Bits == cellId.m_Bits && m_Level == cellId.m_Level;
  }

  inline bool operator != (CellId const & cellId) const
  {
    return !(*this == cellId);
  }

  pair<uint32_t, uint32_t> XY() const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    uint32_t offset = 1 << (DEPTH_LEVELS - 1 - m_Level);
    pair<uint32_t, uint32_t> xy(offset, offset);
    int64_t bits = m_Bits;
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

  uint32_t Radius() const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    return 1 << (DEPTH_LEVELS - 1 - m_Level);
  }

  int Level() const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    return m_Level;
  }

  static CellId FromXY(uint32_t x, uint32_t y, int level = DEPTH_LEVELS - 1)
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
    int64_t bits = 0;
    for (int i = 0; i < level; ++i)
      bits |= ((int64_t((y >> i) & 1) << (2 * i + 1)) | (int64_t((x >> i) & 1) << (2 * i)));
    return CellId(bits, level);
  }

  int64_t SubTreeSize() const
  {
    ASSERT(IsValid(), (m_Bits, m_Level));
    return ((1ULL << 2 * (DEPTH_LEVELS - m_Level)) - 1) / 3ULL;
  }

  struct LessQueueOrder
  {
    bool operator() (CellId<kDepthLevels> const & id1, CellId<kDepthLevels> const & id2) const
    {
      if (id1.m_Level != id2.m_Level)
        return id1.m_Level < id2.m_Level;
      if (id1.m_Bits != id2.m_Bits)
        return id1.m_Bits < id2.m_Bits;
      return false;
    }
  };

  struct LessStackOrder
  {
    bool operator() (CellId<kDepthLevels> const & id1, CellId<kDepthLevels> const & id2) const
    {
      return id1.ToInt64() < id2.ToInt64();
    }
  };

private:

  inline static int64_t SubTreeSize(int level)
  {
    ASSERT_GREATER_OR_EQUAL(level, 0, ());
    static int64_t const kSubTreeSize[] =
    {
      1LL,5LL,21LL,85LL,341LL,1365LL,5461LL,21845LL,87381LL,349525LL,1398101LL,5592405LL,22369621LL,
      89478485LL,357913941LL,1431655765LL,5726623061LL,22906492245LL,91625968981LL,366503875925LL,
      1466015503701LL,5864062014805LL,23456248059221LL,93824992236885LL,375299968947541LL,
      1501199875790165LL,6004799503160661LL,24019198012642645LL,96076792050570581LL,
      384307168202282325LL,1537228672809129301LL,6148914691236517205LL
    };
    return kSubTreeSize[level];
  }

  CellId(int64_t bits, int level) : m_Bits(bits), m_Level(level)
  {
    ASSERT_LESS(level, static_cast<uint32_t>(DEPTH_LEVELS), (bits, level));
    ASSERT_LESS(static_cast<uint64_t>(bits), 1ULL << m_Level * 2, (bits, m_Level));
    ASSERT(IsValid(), (m_Bits, m_Level));
  }

  bool IsValid() const
  {
    if (m_Level >= DEPTH_LEVELS) return false;
    if (static_cast<uint64_t>(m_Bits) >= (1ULL << m_Level * 2)) return false;
    return true;
  }

  int64_t m_Bits;
  int m_Level;
};
#pragma pack(pop)

template <int DEPTH_LEVELS> string debug_print(CellId<DEPTH_LEVELS> const & id)
{
  ostringstream out;
  out << "CellId<" << DEPTH_LEVELS << ">(\"" << id.ToString().c_str() << "\")";
  return out.str();
}

}
