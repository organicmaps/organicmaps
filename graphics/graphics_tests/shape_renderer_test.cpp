#include "testing/testing.hpp"
#include "graphics/shape_renderer.hpp"
#include "base/math.hpp"

using namespace graphics;

namespace
{
  void TestPoints(m2::PointD const & center, float r, vector<m2::PointD> const & points)
  {
    for (size_t i = 0; i < points.size(); ++i)
    {
      TEST_LESS(fabs(center.Length(points[i]) - r), 0.0001, (points[i]));
    }
  }
}

UNIT_TEST(ApproximateArc_Smoke)
{
  m2::PointD const center(1, 2);
  float r = 10;
  vector<m2::PointD> points;
  ShapeRenderer::approximateArc(center, 0, math::pi / 4, r, points);
  TestPoints(center, r, points);
}

UNIT_TEST(ApproximateArc_Crash)
{
  // this test gives only two result points on the device,
  // and the second one is NAN inside ShapeRenderer::fillSector()
  m2::PointD const center(511.7547811565455, 511.84095156751573);
  double const startA = -1.5707963267948966;
  double const endA = -1.6057029118347832;
  double const r = 127.99998027132824;
  vector<m2::PointD> points;
  ShapeRenderer::approximateArc(center, startA, endA, r, points);
  TestPoints(center, r, points);
}

//UNIT_TEST(ApproximateArc_TooManyPoints)
//{
//  m2::PointD const center(10, 10);
//  double const startA = 10;
//  double const endA = 0.1;
//  double const r = 10;
//}
