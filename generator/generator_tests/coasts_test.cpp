#include "../../testing/testing.hpp"

#include "../feature_builder.hpp"

#include "../../indexer/mercator.hpp"
#include "../../indexer/cell_id.hpp"

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

/*
namespace
{
  class DoGetCoasts
  {
    vector<string> const & m_vID;

    bool Has(string const & id) const
    {
      return (find(m_vID.begin(), m_vID.end(), id) != m_vID.end());
    }

    list<m2::RectD> m_rects;

    typedef list<vector<m2::PointD> > polygons_t;
    polygons_t m_maxPoly;

  public:
    DoGetCoasts(vector<string> const & vID) : m_vID(vID) {}

    void operator() (FeatureBuilder1 const & fb, uint64_t)
    {
      int64_t dummy;
      CHECK(fb.GetCoastCell(dummy), ());

      string const id = fb.GetName();
      if (Has(id))
      {
        LOG(LINFO, ("ID = ", id, "Rect = ", fb.GetLimitRect(),
                    "Polygons = ", fb.GetPolygonsCount()));

        m_rects.push_back(fb.GetLimitRect());

        polygons_t const & poly = reinterpret_cast<FeatureBuilder2 const &>(fb).GetPolygons();

        // get polygon with max points count
        size_t maxCount = 0;
        m_maxPoly.push_back(vector<m2::PointD>());
        for (polygons_t::const_iterator i = poly.begin(); i != poly.end(); ++i)
          if (i->size() > maxCount)
          {
            maxCount = i->size();
            m_maxPoly.back() = *i;
          }
      }
    }

    void DumpMaxPolygons()
    {
      LOG(LINFO, ("Original"));
      for (polygons_t::const_iterator i = m_maxPoly.begin(); i != m_maxPoly.end(); ++i)
      {
        m2::RectD r;
        feature::CalcRect(*i, r);
        LOG(LINFO, ("Polygon points count = ", i->size(), "Polygon rect = ", r));
      }

      LOG(LINFO, ("Simplified"));
      /// @todo Check simplified polygons
    }
  };
}

UNIT_TEST(WorldCoasts_CheckBounds)
{
  vector<string> vID;
  vID.push_back("2213023");
  vID.push_back("2213021");

  DoGetCoasts doGet(vID);
  feature::ForEachFromDatRawFormat(
        "/Users/alena/omim/omim-indexer-tmp/WorldCoasts.mwm", doGet);

  doGet.DumpMaxPolygons();
}
*/
