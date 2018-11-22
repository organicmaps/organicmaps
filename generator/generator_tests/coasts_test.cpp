#include "testing/testing.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/feature_helpers.hpp"

#include "coding/point_coding.hpp"

#include "geometry/cellid.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "indexer/cell_id.hpp"
#include "indexer/scales.hpp"

#include "base/logging.hpp"

#include <string>
#include <vector>

using namespace std;

namespace
{
m2::PointU D2I(double x, double y) { return PointDToPointU(m2::PointD(x, y), kPointCoordBits); }

class ProcessCoastsBase
{
public:
  explicit ProcessCoastsBase(vector<string> const & vID) : m_vID(vID) {}

protected:
  bool HasID(FeatureBuilder1 const & fb) const
  {
    TEST(fb.IsCoastCell(), ());
    return (find(m_vID.begin(), m_vID.end(), fb.GetName()) != m_vID.end());
  }

private:
  vector<string> const & m_vID;
};

class DoPrintCoasts : public ProcessCoastsBase
{
public:
  explicit DoPrintCoasts(vector<string> const & vID) : ProcessCoastsBase(vID) {}

  void operator()(FeatureBuilder1 const & fb1, uint64_t)
  {
    if (HasID(fb1))
    {
      FeatureBuilder2 const & fb2 = reinterpret_cast<FeatureBuilder2 const &>(fb1);

      // Check common params.
      TEST(fb2.IsArea(), ());
      int const upperScale = scales::GetUpperScale();
      TEST(fb2.IsDrawableInRange(0, upperScale), ());

      m2::RectD const rect = fb2.GetLimitRect();
      LOG(LINFO, ("ID =", fb1.GetName(), "Rect =", rect, "Polygons =", fb2.GetGeometry()));

      // Make bound rect inflated a little.
      feature::DistanceToSegmentWithRectBounds distFn(rect);
      m2::RectD const boundRect = m2::Inflate(rect, distFn.GetEpsilon(), distFn.GetEpsilon());

      using Points = vector<m2::PointD>;
      using Polygons = list<Points>;

      Polygons const & poly = fb2.GetGeometry();

      // Check that all simplifications are inside bound rect.
      for (int level = 0; level <= upperScale; ++level)
      {
        TEST(fb2.IsDrawableInRange(level, level), ());

        for (auto const & rawPts : poly)
        {
          Points pts;
          feature::SimplifyPoints(distFn, level, rawPts, pts);

          LOG(LINFO, ("Simplified. Level =", level, "Points =", pts));

          for (auto const & p : pts)
            TEST(boundRect.IsPointInside(p), (p));
        }
      }
    }
  }
};

class DoCopyCoasts : public ProcessCoastsBase
{
public:
  DoCopyCoasts(string const & fName, vector<string> const & vID)
    : ProcessCoastsBase(vID), m_collector(fName)
  {
  }

  void operator()(FeatureBuilder1 const & fb1, uint64_t)
  {
    if (HasID(fb1))
      m_collector(fb1);
  }

private:
  feature::FeaturesCollector m_collector;
};
}  // namespace

UNIT_TEST(CellID_CheckRectPoints)
{
  int const level = 6;
  int const count = 1 << 2 * level;

  using Id = m2::CellId<19>;
  using Converter = CellIdConverter<MercatorBounds, Id>;

  for (size_t i = 0; i < count; ++i)
  {
    Id const cell = Id::FromBitsAndLevel(i, level);
    pair<uint32_t, uint32_t> const xy = cell.XY();
    uint32_t const r = 2*cell.Radius();
    uint32_t const bound = (1 << level) * r;

    double minX, minY, maxX, maxY;
    Converter::GetCellBounds(cell, minX, minY, maxX, maxY);

    double minX_, minY_, maxX_, maxY_;
    if (xy.first > r)
    {
      Id neighbour = Id::FromXY(xy.first - r, xy.second, level);
      Converter::GetCellBounds(neighbour, minX_, minY_, maxX_, maxY_);

      TEST_ALMOST_EQUAL_ULPS(minX, maxX_, ());
      TEST_ALMOST_EQUAL_ULPS(minY, minY_, ());
      TEST_ALMOST_EQUAL_ULPS(maxY, maxY_, ());

      TEST_EQUAL(D2I(minX, minY), D2I(maxX_, minY_), ());
      TEST_EQUAL(D2I(minX, maxY), D2I(maxX_, maxY_), ());
    }

    if (xy.first + r < bound)
    {
      Id neighbour = Id::FromXY(xy.first + r, xy.second, level);
      Converter::GetCellBounds(neighbour, minX_, minY_, maxX_, maxY_);

      TEST_ALMOST_EQUAL_ULPS(maxX, minX_, ());
      TEST_ALMOST_EQUAL_ULPS(minY, minY_, ());
      TEST_ALMOST_EQUAL_ULPS(maxY, maxY_, ());

      TEST_EQUAL(D2I(maxX, minY), D2I(minX_, minY_), ());
      TEST_EQUAL(D2I(maxX, maxY), D2I(minX_, maxY_), ());
    }

    if (xy.second > r)
    {
      Id neighbour = Id::FromXY(xy.first, xy.second - r, level);
      Converter::GetCellBounds(neighbour, minX_, minY_, maxX_, maxY_);

      TEST_ALMOST_EQUAL_ULPS(minY, maxY_, ());
      TEST_ALMOST_EQUAL_ULPS(minX, minX_, ());
      TEST_ALMOST_EQUAL_ULPS(maxX, maxX_, ());

      TEST_EQUAL(D2I(minX, minY), D2I(minX_, maxY_), ());
      TEST_EQUAL(D2I(maxX, minY), D2I(maxX_, maxY_), ());
    }

    if (xy.second + r < bound)
    {
      Id neighbour = Id::FromXY(xy.first, xy.second + r, level);
      Converter::GetCellBounds(neighbour, minX_, minY_, maxX_, maxY_);

      TEST_ALMOST_EQUAL_ULPS(maxY, minY_, ());
      TEST_ALMOST_EQUAL_ULPS(minX, minX_, ());
      TEST_ALMOST_EQUAL_ULPS(maxX, maxX_, ());

      TEST_EQUAL(D2I(minX, maxY), D2I(minX_, minY_), ());
      TEST_EQUAL(D2I(maxX, maxY), D2I(maxX_, minY_), ());
    }
  }
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
