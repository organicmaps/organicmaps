#include "../../testing/testing.hpp"

#include "../feature_builder.hpp"
#include "../feature_sorter.hpp"

#include "../../indexer/mercator.hpp"
#include "../../indexer/cell_id.hpp"
#include "../../indexer/scales.hpp"

#include "../../geometry/cellid.hpp"

#include "../../base/logging.hpp"


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

namespace
{
  class DoPrintCoasts
  {
    vector<string> const & m_vID;

    bool Has(string const & id) const
    {
      return (find(m_vID.begin(), m_vID.end(), id) != m_vID.end());
    }

  public:
    DoPrintCoasts(vector<string> const & vID) : m_vID(vID) {}

    void operator() (FeatureBuilder1 const & fb1, uint64_t)
    {
      int64_t dummy;
      TEST(fb1.GetCoastCell(dummy), ());

      string const id = fb1.GetName();
      if (Has(id))
      {
        FeatureBuilder2 const & fb2 = reinterpret_cast<FeatureBuilder2 const &>(fb1);

        // Check common params.
        TEST(fb2.IsArea(), ());
        int const upperScale = scales::GetUpperScale();
        TEST(fb2.IsDrawableInRange(0, upperScale), ());

        m2::RectD const rect = fb2.GetLimitRect();
        LOG(LINFO, ("ID = ", id, "Rect = ", rect, "Polygons = ", fb2.GetPolygons()));

        // Make bound rect inflated a little.
        feature::BoundsDistance dist(rect);
        m2::RectD const boundRect = m2::Inflate(rect, dist.GetEpsilon(), dist.GetEpsilon());

        typedef vector<m2::PointD> PointsT;
        typedef list<PointsT> PolygonsT;

        PolygonsT const & poly = fb2.GetPolygons();

        // Check that all simplifications are inside bound rect.
        for (int level = 0; level <= upperScale; ++level)
        {
          for (PolygonsT::const_iterator i = poly.begin(); i != poly.end(); ++i)
          {
            PointsT pts;
            feature::SimplifyPoints(dist, *i, pts, level);

            LOG(LINFO, ("Simplified. Level = ", level, "Points = ", pts));

            for (size_t j = 0; j < pts.size(); ++j)
              TEST(boundRect.IsPointInside(pts[j]), (pts[j]));
          }
        }
      }
    }
  };
}

/*
UNIT_TEST(WorldCoasts_CheckBounds)
{
  vector<string> vID;
  vID.push_back("1231");
  vID.push_back("123203");
  vID.push_back("12323");
  vID.push_back("03321");

  DoPrintCoasts doGet(vID);
  feature::ForEachFromDatRawFormat(
        "/Users/alena/omim/omim-indexer-tmp/WorldCoasts.mwm.tmp", doGet);
}
*/
