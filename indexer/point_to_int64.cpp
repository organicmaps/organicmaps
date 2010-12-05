#include "cell_id.hpp"

#include "../geometry/pointu_to_uint64.hpp"

#include "../base/bits.hpp"

#include "../std/algorithm.hpp"

#include "../base/start_mem_debug.hpp"


#define POINT_COORD_BITS 30

int64_t PointToInt64(CoordT x, CoordT y)
{
  if (x < MercatorBounds::minX) x = MercatorBounds::minX;
  if (y < MercatorBounds::minY) y = MercatorBounds::minY;
  if (x > MercatorBounds::maxX) x = MercatorBounds::maxX;
  if (y > MercatorBounds::maxY) y = MercatorBounds::maxY;
  uint32_t const ix = static_cast<uint32_t>(0.5 + (x - MercatorBounds::minX)
                      / (MercatorBounds::maxX - MercatorBounds::minX) * (1 << POINT_COORD_BITS));
  uint32_t const iy = static_cast<uint32_t>(0.5 + (y - MercatorBounds::minY)
                      / (MercatorBounds::maxY - MercatorBounds::minY) * (1 << POINT_COORD_BITS));
  int64_t res =  static_cast<int64_t>(m2::PointUToUint64(m2::PointU(ix, iy)));
  ASSERT_LESS_OR_EQUAL(ix, 1 << POINT_COORD_BITS, ());
  ASSERT_LESS_OR_EQUAL(iy, 1 << POINT_COORD_BITS, ());
  ASSERT_LESS_OR_EQUAL(res, 3ULL << 2 * POINT_COORD_BITS, ());
  ASSERT_GREATER_OR_EQUAL(res, 0,
                          ("Highest bits of (ix, iy) are not used, so res should be > 0."));
  return res;
}

CoordPointT Int64ToPoint(int64_t v)
{
  ASSERT_LESS_OR_EQUAL(v, 3ULL << 2 * POINT_COORD_BITS, ());
  m2::PointU const pt = m2::Uint64ToPointU(static_cast<uint64_t>(v));
  CoordT const fx = static_cast<CoordT>(pt.x);
  CoordT const fy = static_cast<CoordT>(pt.y);
  return CoordPointT(
       fx * (MercatorBounds::maxX - MercatorBounds::minX)
         / (1 << POINT_COORD_BITS) + MercatorBounds::minX,
       fy * (MercatorBounds::maxY - MercatorBounds::minY)
         / (1 << POINT_COORD_BITS) + MercatorBounds::minY);
}

pair<int64_t, int64_t> RectToInt64(m2::RectD const & r)
{
  int64_t const p1 = PointToInt64(r.minX(), r.minY());
  int64_t const p2 = PointToInt64(r.maxX(), r.maxY());
  return make_pair(p1, p2);
}

m2::RectD Int64ToRect(pair<int64_t, int64_t> const & p)
{
  CoordPointT const pt1 = Int64ToPoint(p.first);
  CoordPointT const pt2 = Int64ToPoint(p.second);
  return m2::RectD(m2::PointD(pt1.first, pt1.second), m2::PointD(pt2.first, pt2.second));
}
