#include "coding/point_to_integer.hpp"

#include "geometry/mercator.hpp"
#include "geometry/pointu_to_uint64.hpp"

#include "base/bits.hpp"

#include <algorithm>

namespace
{
inline double CoordSize(uint32_t coordBits) { return (1 << coordBits) - 1; }
}

m2::PointU PointD2PointU(double x, double y, uint32_t coordBits)
{
  x = my::clamp(x, MercatorBounds::minX, MercatorBounds::maxX);
  y = my::clamp(y, MercatorBounds::minY, MercatorBounds::maxY);

  uint32_t const ix = static_cast<uint32_t>(0.5 +
                                            (x - MercatorBounds::minX) /
                                                (MercatorBounds::maxX - MercatorBounds::minX) *
                                                CoordSize(coordBits));
  uint32_t const iy = static_cast<uint32_t>(0.5 +
                                            (y - MercatorBounds::minY) /
                                                (MercatorBounds::maxY - MercatorBounds::minY) *
                                                CoordSize(coordBits));

  ASSERT_LESS_OR_EQUAL(ix, CoordSize(coordBits), ());
  ASSERT_LESS_OR_EQUAL(iy, CoordSize(coordBits), ());

  return m2::PointU(ix, iy);
}

int64_t PointToInt64(double x, double y, uint32_t coordBits)
{
  int64_t const res = static_cast<int64_t>(m2::PointUToUint64(PointD2PointU(x, y, coordBits)));

  ASSERT_LESS_OR_EQUAL(res, 3ULL << 2 * POINT_COORD_BITS, ());
  ASSERT_GREATER_OR_EQUAL(res, 0, ("Highest bits of (ix, iy) are not used, so res should be > 0."));
  return res;
}

m2::PointD PointU2PointD(m2::PointU const & pt, uint32_t coordBits)
{
  return m2::PointD(static_cast<double>(pt.x) * (MercatorBounds::maxX - MercatorBounds::minX) /
                            CoordSize(coordBits) +
                        MercatorBounds::minX,
                    static_cast<double>(pt.y) * (MercatorBounds::maxY - MercatorBounds::minY) /
                            CoordSize(coordBits) +
                        MercatorBounds::minY);
}

m2::PointD Int64ToPoint(int64_t v, uint32_t coordBits)
{
  ASSERT_LESS_OR_EQUAL(v, 3ULL << 2 * POINT_COORD_BITS, ());
  return PointU2PointD(m2::Uint64ToPointU(static_cast<uint64_t>(v)), coordBits);
}

std::pair<int64_t, int64_t> RectToInt64(m2::RectD const & r, uint32_t coordBits)
{
  int64_t const p1 = PointToInt64(r.minX(), r.minY(), coordBits);
  int64_t const p2 = PointToInt64(r.maxX(), r.maxY(), coordBits);
  return std::make_pair(p1, p2);
}

m2::RectD Int64ToRect(std::pair<int64_t, int64_t> const & p, uint32_t coordBits)
{
  m2::PointD const pt1 = Int64ToPoint(p.first, coordBits);
  m2::PointD const pt2 = Int64ToPoint(p.second, coordBits);
  return m2::RectD(pt1, pt2);
}

uint32_t DoubleToUint32(double x, double min, double max, uint32_t coordBits)
{
  ASSERT_GREATER_OR_EQUAL(coordBits, 1, ());
  ASSERT_LESS_OR_EQUAL(coordBits, 32, ());
  x = my::clamp(x, min, max);
  return static_cast<uint32_t>(0.5 + (x - min) / (max - min) * bits::GetFullMask(static_cast<uint8_t>(coordBits)));
}

double Uint32ToDouble(uint32_t x, double min, double max, uint32_t coordBits)
{
  ASSERT_GREATER_OR_EQUAL(coordBits, 1, ());
  ASSERT_LESS_OR_EQUAL(coordBits, 32, ());
  return min + static_cast<double>(x) * (max - min) / bits::GetFullMask(static_cast<uint8_t>(coordBits));
}
