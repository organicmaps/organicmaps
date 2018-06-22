#include "coding/point_to_integer.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"

int64_t PointToInt64Obsolete(double x, double y, uint32_t coordBits)
{
  int64_t const res = static_cast<int64_t>(PointUToUint64Obsolete(PointDToPointU(x, y, coordBits)));

  ASSERT_GREATER_OR_EQUAL(res, 0, ("Highest bits of (ix, iy) are not used, so res should be > 0."));
  ASSERT_LESS_OR_EQUAL(static_cast<uint64_t>(res), uint64_t{3} << 2 * POINT_COORD_BITS, ());
  return res;
}

int64_t PointToInt64Obsolete(m2::PointD const & pt, uint32_t coordBits)
{
  return PointToInt64Obsolete(pt.x, pt.y, coordBits);
}

m2::PointD Int64ToPointObsolete(int64_t v, uint32_t coordBits)
{
  ASSERT_GREATER_OR_EQUAL(v, 0, ("Highest bits of (ix, iy) are not used, so res should be > 0."));
  ASSERT_LESS_OR_EQUAL(static_cast<uint64_t>(v), uint64_t{3} << 2 * POINT_COORD_BITS, ());
  return PointUToPointD(Uint64ToPointUObsolete(static_cast<uint64_t>(v)), coordBits);
}

std::pair<int64_t, int64_t> RectToInt64Obsolete(m2::RectD const & r, uint32_t coordBits)
{
  int64_t const p1 = PointToInt64Obsolete(r.minX(), r.minY(), coordBits);
  int64_t const p2 = PointToInt64Obsolete(r.maxX(), r.maxY(), coordBits);
  return std::make_pair(p1, p2);
}

m2::RectD Int64ToRectObsolete(std::pair<int64_t, int64_t> const & p, uint32_t coordBits)
{
  m2::PointD const pt1 = Int64ToPointObsolete(p.first, coordBits);
  m2::PointD const pt2 = Int64ToPointObsolete(p.second, coordBits);
  return m2::RectD(pt1, pt2);
}

uint64_t PointUToUint64Obsolete(m2::PointU const & pt)
{
  uint64_t const res = bits::BitwiseMerge(pt.x, pt.y);
  ASSERT_EQUAL(pt, Uint64ToPointUObsolete(res), ());
  return res;
}

m2::PointU Uint64ToPointUObsolete(int64_t v)
{
  m2::PointU res;
  bits::BitwiseSplit(v, res.x, res.y);
  return res;
}
