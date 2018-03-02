#pragma once
#include "coding/point_to_integer.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "base/assert.hpp"

using RectId = m2::CellId<19>;

// 24 is enough to have cell size < 2.5m * 2.5m for world.
using LocalityCellId = m2::CellId<24>;

template <typename Bounds, typename CellId>
class CellIdConverter
{
public:
  static double XToCellIdX(double x)
  {
    return (x - Bounds::minX) / StepX();
  }
  static double YToCellIdY(double y)
  {
    return (y - Bounds::minY) / StepY();
  }

  static double CellIdXToX(double  x)
  {
    return (x*StepX() + Bounds::minX);
  }
  static double CellIdYToY(double y)
  {
    return (y*StepY() + Bounds::minY);
  }

  static CellId ToCellId(double x, double y)
  {
    uint32_t const ix = static_cast<uint32_t>(XToCellIdX(x));
    uint32_t const iy = static_cast<uint32_t>(YToCellIdY(y));
    CellId id = CellId::FromXY(ix, iy, CellId::DEPTH_LEVELS - 1);
#if 0 // DEBUG
    pair<uint32_t, uint32_t> ixy = id.XY();
    ASSERT(Abs(ixy.first  - ix) <= 1, (x, y, id, ixy));
    ASSERT(Abs(ixy.second - iy) <= 1, (x, y, id, ixy));
    CoordT minX, minY, maxX, maxY;
    GetCellBounds(id, minX, minY, maxX, maxY);
    ASSERT(minX <= x && x <= maxX, (x, y, id, minX, minY, maxX, maxY));
    ASSERT(minY <= y && y <= maxY, (x, y, id, minX, minY, maxX, maxY));
#endif
    return id;
  }

  static CellId Cover2PointsWithCell(double x1, double y1, double x2, double y2)
  {
    CellId id1 = ToCellId(x1, y1);
    CellId id2 = ToCellId(x2, y2);
    while (id1 != id2)
    {
      id1 = id1.Parent();
      id2 = id2.Parent();
    }
#if 0 // DEBUG
    double minX, minY, maxX, maxY;
    GetCellBounds(id1, minX, minY, maxX, maxY);
    ASSERT(my::between_s(minX, maxX, x1), (x1, minX, maxX));
    ASSERT(my::between_s(minX, maxX, x2), (x2, minX, maxX));
    ASSERT(my::between_s(minY, maxY, y1), (y1, minY, maxY));
    ASSERT(my::between_s(minY, maxY, y2), (y2, minY, maxY));
#endif
    return id1;
  }

  static m2::PointD FromCellId(CellId id)
  {
    pair<uint32_t, uint32_t> const xy = id.XY();
    return m2::PointD(CellIdXToX(xy.first), CellIdYToY(xy.second));
  }

  static void GetCellBounds(CellId id,
                            double & minX, double & minY, double & maxX, double & maxY)
  {
    pair<uint32_t, uint32_t> const xy = id.XY();
    uint32_t const r = id.Radius();
    minX = (xy.first - r) * StepX() + Bounds::minX;
    maxX = (xy.first + r) * StepX() + Bounds::minX;
    minY = (xy.second - r) * StepY() + Bounds::minY;
    maxY = (xy.second + r) * StepY() + Bounds::minY;
  }

private:
  inline static double StepX()
  {
    return double(Bounds::maxX - Bounds::minX) / CellId::MAX_COORD;
  }
  inline static double StepY()
  {
    return double(Bounds::maxY - Bounds::minY) / CellId::MAX_COORD;
  }
};
