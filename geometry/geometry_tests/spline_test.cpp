#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"
#include "equality.hpp"
#include "../spline.hpp"

using m2::Spline;
using m2::PointF;

void TestPointFDir(PointF const & dst, PointF const & src)
{
  float len1 = dst.Length();
  float len2 = src.Length();
  TEST_ALMOST_EQUAL(dst.x/len1, src.x/len2, ());
  TEST_ALMOST_EQUAL(dst.y/len1, src.y/len2, ());
}

UNIT_TEST(SmoothedDirections)
{
  vector<PointF> path;
  path.push_back(PointF(0, 0));
  path.push_back(PointF(40, 40));
  path.push_back(PointF(80, 0));

  Spline spl(path);
  float const sqrt2 = sqrtf(2.0f);
  Spline::iterator itr;
  PointF dir1(sqrt2 / 2.0f, sqrt2 / 2.0f);
  PointF dir2(sqrt2 / 2.0f, -sqrt2 / 2.0f);
  itr.Attach(spl);
  TestPointFDir(itr.m_avrDir, dir1);
  itr.Step(sqrt2 * 30.0f);
  TestPointFDir(itr.m_avrDir, dir1);
  itr.Step(sqrt2 * 40.0f);
  TestPointFDir(itr.m_avrDir, dir1 * 0.25f + dir2 * 0.75f);
  itr.Step(sqrt2 * 10.0f);
  TestPointFDir(itr.m_avrDir, dir2);

  path.clear();

  path.push_back(PointF(0, 0));
  path.push_back(PointF(40, 40));
  path.push_back(PointF(80, 40));
  path.push_back(PointF(120, 0));

  PointF dir12(1.0f, 0.0f);
  Spline spl2(path);
  itr.Attach(spl2);
  TestPointFDir(itr.m_avrDir, dir1);
  itr.Step(sqrt2 * 80.0f + 40.0f);
  TestPointFDir(itr.m_avrDir, dir12);
  itr.Attach(spl2);
  itr.Step(sqrt2 * 40.0f);
  TestPointFDir(itr.m_avrDir, dir1);
  itr.Step(80.0f);
  TestPointFDir(itr.m_avrDir, dir12 * 0.5f + dir2 * 0.5f);
}

UNIT_TEST(UsualDirections)
{
  vector<PointF> path;
  path.push_back(PointF(0, 0));
  path.push_back(PointF(40, 40));
  path.push_back(PointF(80, 0));

  Spline spl(path);
  float const sqrt2 = sqrtf(2.0f);
  Spline::iterator itr;
  PointF dir1(sqrt2 / 2.0f, sqrt2 / 2.0f);
  PointF dir2(sqrt2 / 2.0f, -sqrt2 / 2.0f);
  itr.Attach(spl);
  TestPointFDir(itr.m_dir, dir1);
  itr.Step(sqrt2 * 30.0f);
  TestPointFDir(itr.m_dir, dir1);
  itr.Step(sqrt2 * 40.0f);
  TestPointFDir(itr.m_dir, dir2);

  path.clear();

  path.push_back(PointF(0, 0));
  path.push_back(PointF(40, 40));
  path.push_back(PointF(80, 40));
  path.push_back(PointF(120, 0));

  PointF dir12(1.0f, 0.0f);
  Spline spl2(path);
  itr.Attach(spl2);
  TestPointFDir(itr.m_dir, dir1);
  itr.Step(sqrt2 * 80.0f + 35.0f);
  TestPointFDir(itr.m_dir, dir2);
  itr.Attach(spl2);
  itr.Step(sqrt2 * 45.0f);
  TestPointFDir(itr.m_dir, dir12);
  itr.Step(80.0f);
  TestPointFDir(itr.m_dir, dir2);
}

UNIT_TEST(Positions)
{
  vector<PointF> path;
  path.push_back(PointF(0, 0));
  path.push_back(PointF(40, 40));
  path.push_back(PointF(80, 0));

  Spline spl0(path);
  Spline spl4;
  spl4 = spl0;
  float const sqrt2 = sqrtf(2.0f);
  Spline::iterator itr;
  itr.Attach(spl0);
  TestPointFDir(itr.m_pos, PointF(0, 0));
  itr.Step(sqrt2 * 40.0f);
  TestPointFDir(itr.m_pos, PointF(40, 40));
  itr.Step(sqrt2 * 40.0f);
  TestPointFDir(itr.m_pos, PointF(80, 0));
  itr.Attach(spl4);
  TestPointFDir(itr.m_pos, PointF(0, 0));
  itr.Step(sqrt2 * 40.0f);
  TestPointFDir(itr.m_pos, PointF(40, 40));
  itr.Step(sqrt2 * 40.0f);
  TestPointFDir(itr.m_pos, PointF(80, 0));

  path.clear();

  path.push_back(PointF(0, 0));
  path.push_back(PointF(40, 40));
  path.push_back(PointF(80, 40));
  path.push_back(PointF(120, 0));

  Spline spl2(path);
  Spline spl3 = spl2;
  itr.Attach(spl3);
  TestPointFDir(itr.m_pos, PointF(0, 0));
  itr.Step(sqrt2 * 80.0f + 40.0f);
  TestPointFDir(itr.m_pos, PointF(120, 0));
  itr.Attach(spl2);
  itr.Step(sqrt2 * 40.0f);
  TestPointFDir(itr.m_pos, PointF(40, 40));
  itr.Step(2.0f);
  TestPointFDir(itr.m_pos, PointF(42, 40));
  itr.Step(20.0f);
  TestPointFDir(itr.m_pos, PointF(62, 40));
  itr.Step(18.0f);
  TestPointFDir(itr.m_pos, PointF(80, 40));
}

UNIT_TEST(BeginAgain)
{
  vector<PointF> path;
  path.push_back(PointF(0, 0));
  path.push_back(PointF(40, 40));
  path.push_back(PointF(80, 0));

  Spline spl(path);
  float const sqrt2 = sqrtf(2.0f);
  Spline::iterator itr;
  PointF dir1(sqrt2 / 2.0f, sqrt2 / 2.0f);
  PointF dir2(sqrt2 / 2.0f, -sqrt2 / 2.0f);
  itr.Attach(spl);
  TEST_EQUAL(itr.BeginAgain(), false, ());
  itr.Step(90.0f);
  TEST_EQUAL(itr.BeginAgain(), false, ());
  itr.Step(90.0f);
  TEST_EQUAL(itr.BeginAgain(), true, ());
  itr.Step(190.0f);
  TEST_EQUAL(itr.BeginAgain(), true, ());

  path.clear();

  path.push_back(PointF(0, 0));
  path.push_back(PointF(40, 40));
  path.push_back(PointF(80, 40));
  path.push_back(PointF(120, 0));

  Spline spl2(path);
  itr.Attach(spl2);
  TEST_EQUAL(itr.BeginAgain(), false, ());
  itr.Step(90.0f);
  TEST_EQUAL(itr.BeginAgain(), false, ());
  itr.Step(90.0f);
  TEST_EQUAL(itr.BeginAgain(), true, ());
  itr.Step(190.0f);
  TEST_EQUAL(itr.BeginAgain(), true, ());
}

UNIT_TEST(Length)
{
  vector<PointF> path;
  PointF const p1(27.5536633f, 64.2492523f);
  PointF const p2(27.5547638f, 64.2474289f);
  PointF const p3(27.5549412f, 64.2471237f);
  PointF const p4(27.5559044f, 64.2456436f);
  PointF const p5(27.556284f, 64.2451782f);
  path.push_back(p1);
  path.push_back(p2);
  path.push_back(p3);
  path.push_back(p4);
  path.push_back(p5);
  Spline spl(path);
  float len1 = spl.GetLength();
  float l1 = p1.Length(p2);
  float l2 = p2.Length(p3);
  float l3 = p3.Length(p4);
  float l4 = p4.Length(p5);
  float len2 = l1 + l2 + l3 + l4;
  TEST_ALMOST_EQUAL(len1, len2, ());
}

