#include "cell_id.hpp"

#include "../geometry/pointu_to_uint64.hpp"

#include "../base/bits.hpp"

#include "../std/algorithm.hpp"

#include "../base/start_mem_debug.hpp"


#define POINT_COORD_BITS 30

namespace
{

inline double CoordSize(uint32_t coordBits) { return (1 << coordBits); }

}

m2::PointU PointD2PointU(CoordT x, CoordT y)
{
  x = my::clamp(x, MercatorBounds::minX, MercatorBounds::maxX);
  y = my::clamp(y, MercatorBounds::minY, MercatorBounds::maxY);

  uint32_t const ix = static_cast<uint32_t>(0.5 + (x - MercatorBounds::minX)
                         / (MercatorBounds::maxX - MercatorBounds::minX) * CoordSize());
  uint32_t const iy = static_cast<uint32_t>(0.5 + (y - MercatorBounds::minY)
                         / (MercatorBounds::maxY - MercatorBounds::minY) * CoordSize());

  ASSERT_LESS_OR_EQUAL(ix, CoordSize(), ());
  ASSERT_LESS_OR_EQUAL(iy, CoordSize(), ());

  return m2::PointU(ix, iy);
}

int64_t PointToInt64(CoordT x, CoordT y)
{
  int64_t const res =  static_cast<int64_t>(m2::PointUToUint64(PointD2PointU(x, y)));

  ASSERT_LESS_OR_EQUAL(res, 3ULL << 2 * POINT_COORD_BITS, ());
  ASSERT_GREATER_OR_EQUAL(res, 0,
                          ("Highest bits of (ix, iy) are not used, so res should be > 0."));
  return res;
}

CoordPointT PointU2PointD(m2::PointU const & pt)
{
  return CoordPointT(
        static_cast<CoordT>(pt.x) * (MercatorBounds::maxX - MercatorBounds::minX)
            / CoordSize() + MercatorBounds::minX,
        static_cast<CoordT>(pt.y) * (MercatorBounds::maxY - MercatorBounds::minY)
            / CoordSize() + MercatorBounds::minY);
}

CoordPointT Int64ToPoint(int64_t v)
{
  ASSERT_LESS_OR_EQUAL(v, 3ULL << 2 * POINT_COORD_BITS, ());
  return PointU2PointD(m2::Uint64ToPointU(static_cast<uint64_t>(v)));
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
