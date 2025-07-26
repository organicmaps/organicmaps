#include "testing/testing.hpp"

#include "geometry/cellid.hpp"
#include "geometry/covering.hpp"
#include "geometry/covering_utils.hpp"
#include "geometry/point2d.hpp"

#include "base/stl_helpers.hpp"

#include <vector>

// TODO: Add covering unit tests here.

namespace covering_test
{
using namespace std;

using CellId = m2::CellId<5>;

UNIT_TEST(CoverTriangle_Simple)
{
  vector<CellId> v;
  covering::Covering<CellId> c(m2::PointD(3 * 2, 3 * 2), m2::PointD(4 * 2, 12 * 2), m2::PointD(14 * 2, 3 * 2));
  c.OutputToVector(v);
  vector<CellId> e;
  e.push_back(CellId("03"));
  e.push_back(CellId("003"));
  e.push_back(CellId("012"));
  e.push_back(CellId("013"));
  e.push_back(CellId("102"));
  e.push_back(CellId("103"));
  e.push_back(CellId("112"));
  e.push_back(CellId("120"));
  e.push_back(CellId("121"));
  e.push_back(CellId("122"));
  e.push_back(CellId("210"));
  e.push_back(CellId("211"));
  e.push_back(CellId("212"));
  e.push_back(CellId("0211"));
  e.push_back(CellId("0213"));
  e.push_back(CellId("0231"));
  e.push_back(CellId("0233"));
  e.push_back(CellId("1130"));
  e.push_back(CellId("1132"));
  e.push_back(CellId("1230"));
  e.push_back(CellId("1300"));
  e.push_back(CellId("2011"));
  e.push_back(CellId("2013"));
  e.push_back(CellId("2031"));
  e.push_back(CellId("2033"));
  e.push_back(CellId("2130"));
  e.push_back(CellId("2211"));
  e.push_back(CellId("2300"));
  e.push_back(CellId("3000"));
  TEST_EQUAL(v, e, ());
}

UNIT_TEST(CoverTriangle_TriangleInsideCell)
{
  vector<CellId> v;
  covering::Covering<CellId> c(m2::PointD(0.1, 0.1), m2::PointD(0.2, 0.2), m2::PointD(0.2, 0.4));
  c.OutputToVector(v);
  vector<CellId> e;
  e.push_back(CellId("0000"));
  TEST_EQUAL(v, e, ());
}

UNIT_TEST(Covering_Append_Simple)
{
  vector<CellId> v1, v2, v3;

  v1.push_back(CellId("012"));
  v2.push_back(CellId("0123"));
  v3.push_back(CellId("012"));

  v1.push_back(CellId("023"));
  v2.push_back(CellId("023"));
  v3.push_back(CellId("023"));

  v1.push_back(CellId("130"));
  v1.push_back(CellId("131"));
  v2.push_back(CellId("133"));
  v1.push_back(CellId("1320"));
  v1.push_back(CellId("1321"));
  v2.push_back(CellId("1322"));
  v2.push_back(CellId("1323"));
  v3.push_back(CellId("13"));

  covering::Covering<CellId> c1(v1);
  c1.Append(covering::Covering<CellId>(v2));
  vector<CellId> v4;
  c1.OutputToVector(v4);
  sort(v4.begin(), v4.end(), CellId::LessPreOrder());
  TEST_EQUAL(v3, v4, ());
}

UNIT_TEST(IntersectCellWithTriangle_EmptyTriangle)
{
  m2::PointD pt(27.0, 31.0);
  TEST_EQUAL(covering::CELL_OBJECT_NO_INTERSECTION, covering::IntersectCellWithTriangle(CellId("0"), pt, pt, pt), ());
  TEST_EQUAL(covering::CELL_OBJECT_NO_INTERSECTION, covering::IntersectCellWithTriangle(CellId("1"), pt, pt, pt), ());
  TEST_EQUAL(covering::CELL_OBJECT_NO_INTERSECTION, covering::IntersectCellWithTriangle(CellId("2"), pt, pt, pt), ());
  TEST_EQUAL(covering::OBJECT_INSIDE_CELL, covering::IntersectCellWithTriangle(CellId("3"), pt, pt, pt), ());
}

UNIT_TEST(Covering_EmptyTriangle)
{
  m2::PointU pt(27, 31);
  m2::PointD ptd(pt);
  CellId const expectedCellId = CellId::FromXY(pt.x, pt.y, CellId::DEPTH_LEVELS - 1);
  TEST_GREATER(expectedCellId.ToInt64(CellId::DEPTH_LEVELS), 5, ());
  covering::Covering<CellId> covering(ptd, ptd, ptd);
  vector<CellId> ids;
  covering.OutputToVector(ids);
  TEST_EQUAL(ids, vector<CellId>(1, expectedCellId), ());
}

UNIT_TEST(Covering_Simplify_Smoke)
{
  vector<CellId> v;
  v.push_back(CellId("03"));
  v.push_back(CellId("020"));
  v.push_back(CellId("021"));
  v.push_back(CellId("022"));
  v.push_back(CellId("0012"));
  covering::Covering<CellId> covering(v);
  v.clear();
  covering.Simplify();
  covering.OutputToVector(v);
  vector<CellId> e;
  e.push_back(CellId("02"));
  e.push_back(CellId("03"));
  e.push_back(CellId("0012"));
  TEST_EQUAL(v, e, ());
}
}  // namespace covering_test
