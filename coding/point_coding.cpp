#include "coding/point_coding.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"
#include "base/math.hpp"

#include <algorithm>

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
  ASSERT_LESS_OR_EQUAL(min, max, ());
  x = base::Clamp(x, min, max);
  return static_cast<uint32_t>(0.5 + (x - min) / (max - min) * bits::GetFullMask(coordBits));
}

double Uint32ToDouble(uint32_t x, double min, double max, uint8_t coordBits)
{
  ASSERT_GREATER_OR_EQUAL(coordBits, 1, ());
  ASSERT_LESS_OR_EQUAL(coordBits, 32, ());
  auto const res = min + static_cast<double>(x) * (max - min) / bits::GetFullMask(coordBits);
  // Clamp to avoid floating point calculation errors.
  return base::Clamp(res, min, max);
}

m2::PointU PointDToPointU(double x, double y, uint8_t coordBits)
{
  x = base::Clamp(x, mercator::Bounds::kMinX, mercator::Bounds::kMaxX);
  y = base::Clamp(y, mercator::Bounds::kMinY, mercator::Bounds::kMaxY);

  uint32_t const ix = static_cast<uint32_t>(
      0.5 + (x - mercator::Bounds::kMinX) / mercator::Bounds::kRangeX * CoordSize(coordBits));
  uint32_t const iy = static_cast<uint32_t>(
      0.5 + (y - mercator::Bounds::kMinY) / mercator::Bounds::kRangeY * CoordSize(coordBits));

  ASSERT_LESS_OR_EQUAL(ix, CoordSize(coordBits), ());
  ASSERT_LESS_OR_EQUAL(iy, CoordSize(coordBits), ());

  return m2::PointU(ix, iy);
}

m2::PointU PointDToPointU(m2::PointD const & pt, uint8_t coordBits)
{
  return PointDToPointU(pt.x, pt.y, coordBits);
}

m2::PointU PointDToPointU(m2::PointD const & pt, uint8_t coordBits, m2::RectD const & limitRect)
{
  ASSERT_GREATER_OR_EQUAL(coordBits, 1, ());
  ASSERT_LESS_OR_EQUAL(coordBits, 32, ());
  auto const x = DoubleToUint32(pt.x, limitRect.minX(), limitRect.maxX(), coordBits);
  auto const y = DoubleToUint32(pt.y, limitRect.minY(), limitRect.maxY(), coordBits);
  return m2::PointU(x, y);
}

m2::PointD PointUToPointD(m2::PointU const & pt, uint8_t coordBits)
{
  return m2::PointD(
      static_cast<double>(pt.x) * mercator::Bounds::kRangeX / CoordSize(coordBits) +
          mercator::Bounds::kMinX,
      static_cast<double>(pt.y) * mercator::Bounds::kRangeY / CoordSize(coordBits) +
          mercator::Bounds::kMinY);
}

m2::PointD PointUToPointD(m2::PointU const & pt, uint8_t coordBits, m2::RectD const & limitRect)
{
  return m2::PointD(Uint32ToDouble(pt.x, limitRect.minX(), limitRect.maxX(), coordBits),
                    Uint32ToDouble(pt.y, limitRect.minY(), limitRect.maxY(), coordBits));
}

uint8_t GetCoordBits(m2::RectD const & limitRect, double accuracy)
{
  auto const range = std::max(limitRect.SizeX(), limitRect.SizeY());
  auto const valuesNumber = 1.0 + range / accuracy;
  for (uint8_t coordBits = 1; coordBits <= 32; ++coordBits)
  {
    if (CoordSize(coordBits) >= valuesNumber)
      return coordBits;
  }
  return 0;
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
