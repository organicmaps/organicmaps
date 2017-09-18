#include "testing/testing.hpp"

#include "geometry/cbox.hpp"
#include "geometry/point2d.hpp"

#include <vector>

using namespace m2;
using namespace std;

namespace
{
UNIT_TEST(CBox_Smoke)
{
  {
    CBox const cbox(vector<PointD>{});
    TEST(cbox.Points().empty(), ());
    TEST(!cbox.IsInside(0, 0), ());
    TEST(!cbox.IsInside(0, 1), ());
    TEST(!cbox.IsInside(1, 0), ());
  }

  {
    vector<PointD> const points = {{PointD(2, 3)}};
    CBox const cbox(points);
    TEST_EQUAL(cbox.Points(), points, ());
    TEST(cbox.IsInside(2, 3), ());
    TEST(!cbox.IsInside(4, 6), ());
    TEST(!cbox.IsInside(0, 0), ());
  }

  {
    vector<PointD> const points = {{PointD(2, 3), PointD(2, 3), PointD(2, 3)}};
    CBox const cbox(points);
    TEST_EQUAL(cbox.Points(), vector<PointD>{{PointD(2, 3)}}, ());
  }

  {
    vector<PointD> const points = {{PointD(1, 1), PointD(1, 2)}};
    CBox const cbox(points);
    TEST_EQUAL(cbox.Points(), points, ());
    TEST(cbox.IsInside(1, 1.5), ());
    TEST(!cbox.IsInside(1, 3), ());
    TEST(!cbox.IsInside(0, 0), ());
  }

  {
    vector<PointD> const points = {
        {PointD(0, 0), PointD(-2, 3), PointD(1, 5), PointD(3, 2), PointD(1, 2), PointD(0, 3)}};
    CBox const cbox(points);
    TEST_EQUAL(cbox.Points(),
               (vector<PointD>{{PointD(-2, 3), PointD(0, 0), PointD(3, 2), PointD(1, 5)}}), ());
    for (auto const & p : points)
      TEST(cbox.IsInside(p), (p));
    TEST(!cbox.IsInside(1, 0), ());
  }

  {
    vector<PointD> const points = {{PointD(0, 0), PointD(1, 0), PointD(0, 5), PointD(1, 5),
                                    PointD(-2, 2), PointD(-2, 3), PointD(3, 2), PointD(3, 3)}};
    vector<PointD> const expected = {
      {PointD(-2.5, 2.5), PointD(0.5, -0.5), PointD(3.5, 2.5), PointD(0.5, 5.5)}};
    CBox const cbox(points);
    TEST_EQUAL(cbox.Points(), expected, ());

    for (auto const & p : points)
      TEST(cbox.IsInside(p), (p));

    TEST(cbox.IsInside(0, 2), ());

    TEST(!cbox.IsInside(2, 0), ());
    TEST(!cbox.IsInside(4, 2), ());
  }
}
}  // namespace
