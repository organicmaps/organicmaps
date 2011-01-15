#pragma once

#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/math.hpp"
#include "../base/stl_add.hpp"

// If polygon with n vertices is a single strip, return the start index of the strip or n otherwise.
template <typename IsVisibleF>
size_t FindSingleStrip(size_t const n, IsVisibleF isVisible)
{
  for (size_t i = 0; i < n; ++i)
  {
    // Searching for a strip only in a single direction, because the opposite direction
    // is traversed from the last vertex of the possible strip.
    size_t a = my::PrevModN(i, n);
    size_t b = my::NextModN(i, n);
    for (size_t j = 2; j < n; ++j)
    {
      ASSERT_NOT_EQUAL(a, b, ());
      if (!isVisible(a, b))
        break;
      if (j & 1)
        a = my::PrevModN(a, n);
      else
        b = my::NextModN(b, n);
    }
    if (a == b)
      return i;
  }
  return n;
}

// Is segment (v, v1) in cone (vPrev, v, vNext)? Orientation CCW.
template <typename PointT> bool IsSegmentInCone(PointT v, PointT v1, PointT vPrev, PointT vNext)
{
  PointT const diff = v1 - v;
  PointT const edgeL = vPrev - v;
  PointT const edgeR = vNext - v;
  double const cpLR = CrossProduct(edgeR, edgeL);
  ASSERT(!my::AlmostEqual(cpLR, 0.0),
         ("vPrev, v, vNext shouldn't be collinear!", edgeL, edgeR, v, v1, vPrev, vNext));
  if (cpLR > 0)
  {
    // vertex is convex
    return CrossProduct(diff, edgeR) < 0 && CrossProduct(diff, edgeL) > 0;
  }
  else
  {
    // vertex is reflex
    return CrossProduct(diff, edgeR) < 0 || CrossProduct(diff, edgeL) > 0;
  }
}

// Is diagonal (i0, i1) visible in polygon [beg, end).
template <typename IterT>
bool IsDiagonalVisible(IterT const beg, IterT const end, IterT const i0, IterT const i1)
{
  // TODO: Orientation CCW!!
  if (!IsSegmentInCone(*i0, *i1, *PrevIterInCycle(i0, beg, end), *NextIterInCycle(i0, beg, end)))
    return false;

  for (IterT j0 = beg, j1 = PrevIterInCycle(beg, beg, end); j0 != end; j1 = j0++)
    if (j0 != i0 && j0 != i1 && j1 != i0 && j1 != i1 && SegmentsIntersect(*i0, *i1, *j0, *j1))
      return false;

  return true;
}

template <typename IterT> class IsDiagonalVisibleFunctor
{
public:
  IsDiagonalVisibleFunctor(IterT const beg, IterT const end) : m_Beg(beg), m_End(end) {}

  bool operator () (size_t a, size_t b) const
  {
    return IsDiagonalVisible(m_Beg, m_End, m_Beg + a, m_End + b);
  }

private:
  IterT m_Beg, m_End;
};
