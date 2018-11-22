#include "coding/point_coding.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"
#include "base/math.hpp"

namespace
{
double CoordSize(uint8_t coordBits)
{
  ASSERT_LESS_OR_EQUAL(coordBits, 32, ());
  return static_cast<double>((uint64_t{1} << coordBits) - 1);
}
}  // namespace

uint32_t DoubleToUint32(double x, double min, double max, uint8_t coordBits)
{
  ASSERT_GREATER_OR_EQUAL(coordBits, 1, ());
  ASSERT_LESS_OR_EQUAL(coordBits, 32, ());
  x = base::clamp(x, min, max);
  return static_cast<uint32_t>(0.5 + (x - min) / (max - min) * bits::GetFullMask(coordBits));
}

double Uint32ToDouble(uint32_t x, double min, double max, uint8_t coordBits)
{
  ASSERT_GREATER_OR_EQUAL(coordBits, 1, ());
  ASSERT_LESS_OR_EQUAL(coordBits, 32, ());
  return min + static_cast<double>(x) * (max - min) / bits::GetFullMask(coordBits);
}

m2::PointU PointDToPointU(double x, double y, uint8_t coordBits)
{
  x = base::clamp(x, MercatorBounds::kMinX, MercatorBounds::kMaxX);
  y = base::clamp(y, MercatorBounds::kMinY, MercatorBounds::kMaxY);

  uint32_t const ix = static_cast<uint32_t>(
      0.5 + (x - MercatorBounds::kMinX) / MercatorBounds::kRangeX * CoordSize(coordBits));
  uint32_t const iy = static_cast<uint32_t>(
      0.5 + (y - MercatorBounds::kMinY) / MercatorBounds::kRangeY * CoordSize(coordBits));

  ASSERT_LESS_OR_EQUAL(ix, CoordSize(coordBits), ());
  ASSERT_LESS_OR_EQUAL(iy, CoordSize(coordBits), ());

  return m2::PointU(ix, iy);
}

m2::PointU PointDToPointU(m2::PointD const & pt, uint8_t coordBits)
{
  return PointDToPointU(pt.x, pt.y, coordBits);
}

m2::PointD PointUToPointD(m2::PointU const & pt, uint8_t coordBits)
{
  return m2::PointD(static_cast<double>(pt.x) * MercatorBounds::kRangeX / CoordSize(coordBits) +
                        MercatorBounds::kMinX,
                    static_cast<double>(pt.y) * MercatorBounds::kRangeY / CoordSize(coordBits) +
                        MercatorBounds::kMinY);
}

// Obsolete functions ------------------------------------------------------------------------------

int64_t PointToInt64Obsolete(double x, double y, uint8_t coordBits)
{
  int64_t const res = static_cast<int64_t>(PointUToUint64Obsolete(PointDToPointU(x, y, coordBits)));

  ASSERT_GREATER_OR_EQUAL(res, 0, ("Highest bits of (ix, iy) are not used, so res should be > 0."));
  ASSERT_LESS_OR_EQUAL(static_cast<uint64_t>(res), uint64_t{3} << 2 * kPointCoordBits, ());
  return res;
}

int64_t PointToInt64Obsolete(m2::PointD const & pt, uint8_t coordBits)
{
  return PointToInt64Obsolete(pt.x, pt.y, coordBits);
}

m2::PointD Int64ToPointObsolete(int64_t v, uint8_t coordBits)
{
  ASSERT_GREATER_OR_EQUAL(v, 0, ("Highest bits of (ix, iy) are not used, so res should be > 0."));
  ASSERT_LESS_OR_EQUAL(static_cast<uint64_t>(v), uint64_t{3} << 2 * kPointCoordBits, ());
  return PointUToPointD(Uint64ToPointUObsolete(static_cast<uint64_t>(v)), coordBits);
}

std::pair<int64_t, int64_t> RectToInt64Obsolete(m2::RectD const & r, uint8_t coordBits)
{
  int64_t const p1 = PointToInt64Obsolete(r.minX(), r.minY(), coordBits);
  int64_t const p2 = PointToInt64Obsolete(r.maxX(), r.maxY(), coordBits);
  return std::make_pair(p1, p2);
}

m2::RectD Int64ToRectObsolete(std::pair<int64_t, int64_t> const & p, uint8_t coordBits)
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
