#include "../cell_coverer.hpp"
#include "../../testing/testing.hpp"

#include "../../geometry/covering.hpp"
#include "../../coding/hex.hpp"
#include "../../base/logging.hpp"


// Unit test uses m2::CellId<30> for historical reasons, the actual production code uses RectId.
typedef m2::CellId<30> CellIdT;
typedef Bounds<-180, -90, 180, 90> OrthoBounds;

namespace
{
  class CoordsPusher
  {
  public:
    typedef vector< CoordPointT > VT;

    CoordsPusher(VT & v) : m_v(v) {}
    CoordsPusher & operator()(CoordT x, CoordT y)
    {
      m_v.push_back(make_pair(x, y));
      return *this;
    }
  private:
    VT & m_v;
  };

  string EnumCells(vector<CellIdT> const & v)
  {
    string result;
    for (size_t i = 0; i < v.size(); ++i)
    {
      result += v[i].ToString();
      if (i != v.size() - 1) result += ' ';
    }
    return result;
  }
}

UNIT_TEST(CellIdToStringRecode)
{
  char const kTest[] = "21032012203";
  TEST_EQUAL(CellIdT::FromString(kTest).ToString(), kTest, ());
}

UNIT_TEST(GoldenTestCover)
{
  vector<CoordPointT> coords;
  CoordsPusher c(coords);
  c(0.7, 0.5)
  (1.5, 1.5)
  (2.5, 3.5)
  (5.5, 5.0);

  vector<CellIdT> cells;
  CoverPolyLine<Bounds<0, 0, 8, 8> >(coords, 3, cells);

  TEST_EQUAL(EnumCells(cells), "000 001 003 021 030 032 033 211 300 301 303", ());
}

UNIT_TEST(GoldenTestCellIntersect)
{
  vector< CoordPointT > coords;
  CoordsPusher c(coords);
   c (0.7, 0.5)
    (1.5, 1.5)
    (2.5, 3.5)
    (5.5, 5.0);

  vector<CellIdT> cells;

  typedef Bounds<0, 0, 8, 8> BoundsT;

  CoverPolyLine<BoundsT>(coords, 7, cells);
  TEST(!CellIntersects<BoundsT>(coords, CellIdT::FromString("210")), ());
}

UNIT_TEST(GoldenOrthoCover)
{
  vector< CoordPointT > coords;
  CoordsPusher c(coords);
  c
  (27.545927047729492, 53.888893127441406)
  (27.546476364135742, 53.888614654541016)
  (27.546852111816406, 53.889347076416016)
  (27.546596527099609, 53.889404296875000)
  (27.545927047729492, 53.888893127441406);

  vector<CellIdT> cells;
  CoverPolyLine<OrthoBounds>(coords, 19, cells);

  TEST_EQUAL(EnumCells(cells),
    "3201221130210310103 3201221130210310120 3201221130210310121 3201221130210310122 "
    "3201221130210310123 3201221130210310301 3201221130210310310", ());
}

UNIT_TEST(GoldenCoverRect)
{
  vector<CellIdT> cells;
  CoverRect<OrthoBounds>(27.43, 53.83, 27.70, 53.96, 4, cells);

  TEST_EQUAL(cells.size(), 4, ());

  TEST_EQUAL(cells[0].ToString(), "32012211300", ());
  TEST_EQUAL(cells[1].ToString(), "32012211301", ());
  TEST_EQUAL(cells[2].ToString(), "32012211302", ());
  TEST_EQUAL(cells[3].ToString(), "32012211303", ());
}

UNIT_TEST(ArtificialCoverRect)
{
  typedef Bounds<0, 0, 16, 16> TestBounds;

  vector<CellIdT> cells;
  CoverRect<TestBounds>(5, 5, 11, 11, 4, cells);

  TEST_EQUAL(cells.size(), 4, ());

  TEST_EQUAL(cells[0].ToString(), "03", ());
  TEST_EQUAL(cells[1].ToString(), "12", ());
  TEST_EQUAL(cells[2].ToString(), "21", ());
  TEST_EQUAL(cells[3].ToString(), "30", ());
}

UNIT_TEST(CoverEmptyTriangleTest)
{
  vector<int64_t> ids, expectedIds;
  m2::PointD pt(8.89748, 51.974);
  expectedIds.push_back(CoverPoint<MercatorBounds, RectId>(CoordPointT(pt.x, pt.y)).ToInt64());
  TEST_NOT_EQUAL(expectedIds[0], 1, ());
  typedef CellIdConverter<MercatorBounds, RectId> CellIdConverterType;
  m2::PointD pt1(CellIdConverterType::XToCellIdX(pt.x), CellIdConverterType::YToCellIdY(pt.y));
  covering::Covering<RectId> covering(pt1, pt1, pt1);
  covering.OutputToVector(ids);
  TEST_EQUAL(ids, expectedIds, ());
}

// TODO: UNIT_TEST(CoverPolygon)
/*
UNIT_TEST(CoverPolygon)
{
  typedef Bounds<0, 0, 16, 16> TestBounds;

  vector< CoordPointT > coords;
  CoordsPusher c(coords);
  c
    (4.1, 4.1)
    (6.1, 8.1)
    (10.1, 10.1)
    (8.1, 6.1)
    (4.1, 4.1);

  vector<CellIdT> cells;
  CoverPolygon<TestBounds>(coords, 4, cells);

  TEST_EQUAL(EnumCells(cells),
    "0300 0301 0302 0303 0312 0313 0321 0330 0331 1220 0323 0332 0333 "
    "1222 1223 2110 2111 3000 3001 2113 3002 3003 3012 3021 3030", ());
}
*/
