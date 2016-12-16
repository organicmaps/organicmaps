#include "testing/testing.hpp"
#include "geometry/geometry_tests/equality.hpp"
#include "geometry/spline.hpp"

using m2::Spline;
using m2::PointD;

void TestPointDDir(PointD const & dst, PointD const & src)
{
  double len1 = dst.Length();
  double len2 = src.Length();
  TEST_ALMOST_EQUAL_ULPS(dst.x/len1, src.x/len2, ());
  TEST_ALMOST_EQUAL_ULPS(dst.y/len1, src.y/len2, ());
}

UNIT_TEST(SmoothedDirections)
{
  vector<PointD> path;
  path.push_back(PointD(0, 0));
  path.push_back(PointD(40, 40));
  path.push_back(PointD(80, 0));

  Spline spl(path);
  double const sqrt2 = sqrt(2.0);
  Spline::iterator itr;
  PointD dir1(sqrt2 / 2.0, sqrt2 / 2.0);
  PointD dir2(sqrt2 / 2.0, -sqrt2 / 2.0);
  itr.Attach(spl);
  TestPointDDir(itr.m_avrDir, dir1);
  itr.Advance(sqrt2 * 30.0);
  TestPointDDir(itr.m_avrDir, dir1);
  itr.Advance(sqrt2 * 40.0);
  TestPointDDir(itr.m_avrDir, dir1 * 0.25 + dir2 * 0.75);
  itr.Advance(sqrt2 * 10.0);
  TestPointDDir(itr.m_avrDir, dir2);

  path.clear();

  path.push_back(PointD(0, 0));
  path.push_back(PointD(40, 40));
  path.push_back(PointD(80, 40));
  path.push_back(PointD(120, 0));

  PointD dir12(1.0, 0.0);
  Spline spl2(path);
  itr.Attach(spl2);
  TestPointDDir(itr.m_avrDir, dir1);
  itr.Advance(sqrt2 * 80.0 + 40.0);
  TestPointDDir(itr.m_avrDir, dir12);
  itr.Attach(spl2);
  itr.Advance(sqrt2 * 40.0);
  TestPointDDir(itr.m_avrDir, dir1);
  itr.Advance(80.0);
  TestPointDDir(itr.m_avrDir, dir12 * 0.5 + dir2 * 0.5);
}

UNIT_TEST(UsualDirections)
{
  vector<PointD> path;
  path.push_back(PointD(0, 0));
  path.push_back(PointD(40, 40));
  path.push_back(PointD(80, 0));

  Spline spl(path);
  double const sqrt2 = sqrtf(2.0);
  Spline::iterator itr;
  PointD dir1(sqrt2 / 2.0, sqrt2 / 2.0);
  PointD dir2(sqrt2 / 2.0, -sqrt2 / 2.0);
  itr.Attach(spl);
  TestPointDDir(itr.m_dir, dir1);
  itr.Advance(sqrt2 * 30.0);
  TestPointDDir(itr.m_dir, dir1);
  itr.Advance(sqrt2 * 40.0);
  TestPointDDir(itr.m_dir, dir2);

  path.clear();

  path.push_back(PointD(0, 0));
  path.push_back(PointD(40, 40));
  path.push_back(PointD(80, 40));
  path.push_back(PointD(120, 0));

  PointD dir12(1.0, 0.0);
  Spline spl2(path);
  itr.Attach(spl2);
  TestPointDDir(itr.m_dir, dir1);
  itr.Advance(sqrt2 * 80.0 + 35.0);
  TestPointDDir(itr.m_dir, dir2);
  itr.Attach(spl2);
  itr.Advance(sqrt2 * 45.0);
  TestPointDDir(itr.m_dir, dir12);
  itr.Advance(80.0);
  TestPointDDir(itr.m_dir, dir2);
}

UNIT_TEST(Positions)
{
  vector<PointD> path;
  path.push_back(PointD(0, 0));
  path.push_back(PointD(40, 40));
  path.push_back(PointD(80, 0));

  Spline spl0(path);
  Spline spl4;
  spl4 = spl0;
  double const sqrt2 = sqrt(2.0);
  Spline::iterator itr;
  itr.Attach(spl0);
  TestPointDDir(itr.m_pos, PointD(0, 0));
  itr.Advance(sqrt2 * 40.0);
  TestPointDDir(itr.m_pos, PointD(40, 40));
  itr.Advance(sqrt2 * 40.0);
  TestPointDDir(itr.m_pos, PointD(80, 0));
  itr.Attach(spl4);
  TestPointDDir(itr.m_pos, PointD(0, 0));
  itr.Advance(sqrt2 * 40.0);
  TestPointDDir(itr.m_pos, PointD(40, 40));
  itr.Advance(sqrt2 * 40.0);
  TestPointDDir(itr.m_pos, PointD(80, 0));

  path.clear();

  path.push_back(PointD(0, 0));
  path.push_back(PointD(40, 40));
  path.push_back(PointD(80, 40));
  path.push_back(PointD(120, 0));

  Spline spl2(path);
  Spline spl3 = spl2;
  itr.Attach(spl3);
  TestPointDDir(itr.m_pos, PointD(0, 0));
  itr.Advance(sqrt2 * 80.0 + 40.0);
  TestPointDDir(itr.m_pos, PointD(120, 0));
  itr.Attach(spl2);
  itr.Advance(sqrt2 * 40.0);
  TestPointDDir(itr.m_pos, PointD(40, 40));
  itr.Advance(2.0);
  TestPointDDir(itr.m_pos, PointD(42, 40));
  itr.Advance(20.0);
  TestPointDDir(itr.m_pos, PointD(62, 40));
  itr.Advance(18.0);
  TestPointDDir(itr.m_pos, PointD(80, 40));
}

UNIT_TEST(BeginAgain)
{
  vector<PointD> path;
  path.push_back(PointD(0, 0));
  path.push_back(PointD(40, 40));
  path.push_back(PointD(80, 0));

  Spline spl(path);
  double const sqrt2 = sqrtf(2.0);
  Spline::iterator itr;
  PointD dir1(sqrt2 / 2.0, sqrt2 / 2.0);
  PointD dir2(sqrt2 / 2.0, -sqrt2 / 2.0);
  itr.Attach(spl);
  TEST_EQUAL(itr.BeginAgain(), false, ());
  itr.Advance(90.0);
  TEST_EQUAL(itr.BeginAgain(), false, ());
  itr.Advance(90.0);
  TEST_EQUAL(itr.BeginAgain(), true, ());
  itr.Advance(190.0);
  TEST_EQUAL(itr.BeginAgain(), true, ());

  path.clear();

  path.push_back(PointD(0, 0));
  path.push_back(PointD(40, 40));
  path.push_back(PointD(80, 40));
  path.push_back(PointD(120, 0));

  Spline spl2(path);
  itr.Attach(spl2);
  TEST_EQUAL(itr.BeginAgain(), false, ());
  itr.Advance(90.0);
  TEST_EQUAL(itr.BeginAgain(), false, ());
  itr.Advance(90.0);
  TEST_EQUAL(itr.BeginAgain(), true, ());
  itr.Advance(190.0);
  TEST_EQUAL(itr.BeginAgain(), true, ());
}

UNIT_TEST(Length)
{
  vector<PointD> path;
  PointD const p1(27.5536633, 64.2492523);
  PointD const p2(27.5547638, 64.2474289);
  PointD const p3(27.5549412, 64.2471237);
  PointD const p4(27.5559044, 64.2456436);
  PointD const p5(27.556284, 64.2451782);
  path.push_back(p1);
  path.push_back(p2);
  path.push_back(p3);
  path.push_back(p4);
  path.push_back(p5);
  Spline spl(path);
  double len1 = spl.GetLength();
  double l1 = p1.Length(p2);
  double l2 = p2.Length(p3);
  double l3 = p3.Length(p4);
  double l4 = p4.Length(p5);
  double len2 = l1 + l2 + l3 + l4;
  TEST_ALMOST_EQUAL_ULPS(len1, len2, ());
}

