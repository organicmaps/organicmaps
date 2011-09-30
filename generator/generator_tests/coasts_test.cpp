#include "../../testing/testing.hpp"

#include "../../indexer/mercator.hpp"
#include "../../indexer/cell_id.hpp"

#include "../../geometry/cellid.hpp"


namespace
{
  inline m2::PointU D2I(double x, double y)
  {
    return PointD2PointU(m2::PointD(x, y), POINT_COORD_BITS);
  }
}

UNIT_TEST(CellID_CheckRectPoints)
{
  int const level = 6;
  int const count = 1 << 2 * level;

  typedef m2::CellId<19> TId;
  typedef CellIdConverter<MercatorBounds, TId> TConverter;

  for (size_t i = 0; i < count; ++i)
  {
    TId const cell = TId::FromBitsAndLevel(i, level);
    pair<uint32_t, uint32_t> const xy = cell.XY();
    uint32_t const r = 2*cell.Radius();
    uint32_t const bound = (1 << level) * r;

    double minX, minY, maxX, maxY;
    TConverter::GetCellBounds(cell, minX, minY, maxX, maxY);

    double minX_, minY_, maxX_, maxY_;
    if (xy.first > r)
    {
      TId neibour = TId::FromXY(xy.first - r, xy.second, level);
      TConverter::GetCellBounds(neibour, minX_, minY_, maxX_, maxY_);

      TEST_ALMOST_EQUAL(minX, maxX_, ());
      TEST_ALMOST_EQUAL(minY, minY_, ());
      TEST_ALMOST_EQUAL(maxY, maxY_, ());

      TEST_EQUAL(D2I(minX, minY), D2I(maxX_, minY_), ());
      TEST_EQUAL(D2I(minX, maxY), D2I(maxX_, maxY_), ());
    }

    if (xy.first + r < bound)
    {
      TId neibour = TId::FromXY(xy.first + r, xy.second, level);
      TConverter::GetCellBounds(neibour, minX_, minY_, maxX_, maxY_);

      TEST_ALMOST_EQUAL(maxX, minX_, ());
      TEST_ALMOST_EQUAL(minY, minY_, ());
      TEST_ALMOST_EQUAL(maxY, maxY_, ());

      TEST_EQUAL(D2I(maxX, minY), D2I(minX_, minY_), ());
      TEST_EQUAL(D2I(maxX, maxY), D2I(minX_, maxY_), ());
    }

    if (xy.second > r)
    {
      TId neibour = TId::FromXY(xy.first, xy.second - r, level);
      TConverter::GetCellBounds(neibour, minX_, minY_, maxX_, maxY_);

      TEST_ALMOST_EQUAL(minY, maxY_, ());
      TEST_ALMOST_EQUAL(minX, minX_, ());
      TEST_ALMOST_EQUAL(maxX, maxX_, ());

      TEST_EQUAL(D2I(minX, minY), D2I(minX_, maxY_), ());
      TEST_EQUAL(D2I(maxX, minY), D2I(maxX_, maxY_), ());
    }

    if (xy.second + r < bound)
    {
      TId neibour = TId::FromXY(xy.first, xy.second + r, level);
      TConverter::GetCellBounds(neibour, minX_, minY_, maxX_, maxY_);

      TEST_ALMOST_EQUAL(maxY, minY_, ());
      TEST_ALMOST_EQUAL(minX, minX_, ());
      TEST_ALMOST_EQUAL(maxX, maxX_, ());

      TEST_EQUAL(D2I(minX, maxY), D2I(minX_, minY_), ());
      TEST_EQUAL(D2I(maxX, maxY), D2I(maxX_, minY_), ());
    }
  }
}
