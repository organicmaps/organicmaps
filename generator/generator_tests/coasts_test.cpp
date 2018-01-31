#include "testing/testing.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/feature_helpers.hpp"

#include "geometry/mercator.hpp"
#include "indexer/cell_id.hpp"
#include "indexer/scales.hpp"

#include "geometry/cellid.hpp"

#include "base/logging.hpp"

using namespace std;

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

      TEST_ALMOST_EQUAL_ULPS(minX, maxX_, ());
      TEST_ALMOST_EQUAL_ULPS(minY, minY_, ());
      TEST_ALMOST_EQUAL_ULPS(maxY, maxY_, ());

      TEST_EQUAL(D2I(minX, minY), D2I(maxX_, minY_), ());
      TEST_EQUAL(D2I(minX, maxY), D2I(maxX_, maxY_), ());
    }

    if (xy.first + r < bound)
    {
      TId neibour = TId::FromXY(xy.first + r, xy.second, level);
      TConverter::GetCellBounds(neibour, minX_, minY_, maxX_, maxY_);

      TEST_ALMOST_EQUAL_ULPS(maxX, minX_, ());
      TEST_ALMOST_EQUAL_ULPS(minY, minY_, ());
      TEST_ALMOST_EQUAL_ULPS(maxY, maxY_, ());

      TEST_EQUAL(D2I(maxX, minY), D2I(minX_, minY_), ());
      TEST_EQUAL(D2I(maxX, maxY), D2I(minX_, maxY_), ());
    }

    if (xy.second > r)
    {
      TId neibour = TId::FromXY(xy.first, xy.second - r, level);
      TConverter::GetCellBounds(neibour, minX_, minY_, maxX_, maxY_);

      TEST_ALMOST_EQUAL_ULPS(minY, maxY_, ());
      TEST_ALMOST_EQUAL_ULPS(minX, minX_, ());
      TEST_ALMOST_EQUAL_ULPS(maxX, maxX_, ());

      TEST_EQUAL(D2I(minX, minY), D2I(minX_, maxY_), ());
      TEST_EQUAL(D2I(maxX, minY), D2I(maxX_, maxY_), ());
    }

    if (xy.second + r < bound)
    {
      TId neibour = TId::FromXY(xy.first, xy.second + r, level);
      TConverter::GetCellBounds(neibour, minX_, minY_, maxX_, maxY_);

      TEST_ALMOST_EQUAL_ULPS(maxY, minY_, ());
      TEST_ALMOST_EQUAL_ULPS(minX, minX_, ());
      TEST_ALMOST_EQUAL_ULPS(maxX, maxX_, ());

      TEST_EQUAL(D2I(minX, maxY), D2I(minX_, minY_), ());
      TEST_EQUAL(D2I(maxX, maxY), D2I(maxX_, minY_), ());
    }
  }
}

namespace
{
  class ProcessCoastsBase
  {
    vector<string> const & m_vID;

  protected:
    bool HasID(FeatureBuilder1 const & fb) const
    {
      TEST(fb.IsCoastCell(), ());
      return (find(m_vID.begin(), m_vID.end(), fb.GetName()) != m_vID.end());
    }

  public:
    ProcessCoastsBase(vector<string> const & vID) : m_vID(vID) {}
  };

  class DoPrintCoasts : public ProcessCoastsBase
  {
  public:
    DoPrintCoasts(vector<string> const & vID) : ProcessCoastsBase(vID) {}

    void operator() (FeatureBuilder1 const & fb1, uint64_t)
    {
      if (HasID(fb1))
      {
        FeatureBuilder2 const & fb2 = reinterpret_cast<FeatureBuilder2 const &>(fb1);

        // Check common params.
        TEST(fb2.IsArea(), ());
        int const upperScale = scales::GetUpperScale();
        TEST(fb2.IsDrawableInRange(0, upperScale), ());

        m2::RectD const rect = fb2.GetLimitRect();
        LOG(LINFO, ("ID = ", fb1.GetName(), "Rect = ", rect, "Polygons = ", fb2.GetGeometry()));

        // Make bound rect inflated a little.
        feature::BoundsDistance dist(rect);
        m2::RectD const boundRect = m2::Inflate(rect, dist.GetEpsilon(), dist.GetEpsilon());

        typedef vector<m2::PointD> PointsT;
        typedef list<PointsT> PolygonsT;

        PolygonsT const & poly = fb2.GetGeometry();

        // Check that all simplifications are inside bound rect.
        for (int level = 0; level <= upperScale; ++level)
        {
          TEST(fb2.IsDrawableInRange(level, level), ());

          for (PolygonsT::const_iterator i = poly.begin(); i != poly.end(); ++i)
          {
            PointsT pts;
            feature::SimplifyPoints(dist, level, *i, pts);

            LOG(LINFO, ("Simplified. Level = ", level, "Points = ", pts));

            for (size_t j = 0; j < pts.size(); ++j)
              TEST(boundRect.IsPointInside(pts[j]), (pts[j]));
          }
        }
      }
    }
  };

  class DoCopyCoasts : public ProcessCoastsBase
  {
    feature::FeaturesCollector m_collector;
  public:
    DoCopyCoasts(string const & fName, vector<string> const & vID)
      : ProcessCoastsBase(vID), m_collector(fName)
    {
    }

    void operator() (FeatureBuilder1 const & fb1, uint64_t)
    {
      if (HasID(fb1))
        m_collector(fb1);
    }
  };
}

/*
UNIT_TEST(WorldCoasts_CheckBounds)
{
  vector<string> vID;

  // bounds
  vID.push_back("2222");
  vID.push_back("3333");
  vID.push_back("0000");
  vID.push_back("1111");

  // bad cells
  vID.push_back("2021");
  vID.push_back("2333");
  vID.push_back("3313");
  vID.push_back("1231");
  vID.push_back("32003");
  vID.push_back("21330");
  vID.push_back("20110");
  vID.push_back("03321");
  vID.push_back("12323");
  vID.push_back("1231");
  vID.push_back("1311");

  //DoPrintCoasts doProcess(vID);
  DoCopyCoasts doProcess("/Users/alena/omim/omim/data/WorldCoasts.mwm.tmp", vID);
  feature::ForEachFromDatRawFormat("/Users/alena/omim/omim-indexer-tmp/WorldCoasts.mwm.tmp", doProcess);
}
*/
