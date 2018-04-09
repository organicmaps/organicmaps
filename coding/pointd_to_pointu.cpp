#include "coding/pointd_to_pointu.hpp"

#include "geometry/mercator.hpp"

#include "base/bits.hpp"
#include "base/math.hpp"

namespace
{
double CoordSize(uint32_t coordBits) { return (1 << coordBits) - 1; }
}  // namespace

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

m2::PointU PointDToPointU(double x, double y, uint32_t coordBits)
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

m2::PointU PointDToPointU(m2::PointD const & pt, uint32_t coordBits)
{
  return PointDToPointU(pt.x, pt.y, coordBits);
}

m2::PointD PointUToPointD(m2::PointU const & pt, uint32_t coordBits)
{
  return m2::PointD(static_cast<double>(pt.x) * (MercatorBounds::maxX - MercatorBounds::minX) /
                            CoordSize(coordBits) +
                        MercatorBounds::minX,
                    static_cast<double>(pt.y) * (MercatorBounds::maxY - MercatorBounds::minY) /
                            CoordSize(coordBits) +
                        MercatorBounds::minY);
}
