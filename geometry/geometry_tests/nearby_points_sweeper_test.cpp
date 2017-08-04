#include "testing/testing.hpp"

#include "geometry/nearby_points_sweeper.hpp"
#include "geometry/point2d.hpp"

#include "base/stl_add.hpp"

#include "std/set.hpp"
#include "std/vector.hpp"

using namespace m2;

namespace search
{
namespace
{
// Multiset is used here to catch situations when some index is reported more than once.
using TIndexSet = multiset<size_t>;

UNIT_TEST(NearbyPointsSweeper_Smoke)
{
  {
    NearbyPointsSweeper sweeper(0.0);
    for (size_t i = 0; i < 10; ++i)
      sweeper.Add(10.0, 10.0, i);

    TIndexSet expected = {0};

    TIndexSet actual;
    sweeper.Sweep(MakeInsertFunctor(actual));

    TEST_EQUAL(expected, actual, ());
  }

  {
    vector<double> const coords = {0.0, 0.5, 1.0, 1.5, 1.4, 1.6};

    {
      NearbyPointsSweeper sweeper(0.5);

      for (size_t i = 0; i < coords.size(); ++i)
        sweeper.Add(coords[i], 0.0, i);

      TIndexSet expected = {0, 2, 5};

      TIndexSet actual;
      sweeper.Sweep(MakeInsertFunctor(actual));

      TEST_EQUAL(expected, actual, ());
    }

    {
      NearbyPointsSweeper sweeper(0.5);

      for (size_t i = 0; i < coords.size(); ++i)
        sweeper.Add(0.0, coords[i], i);

      TIndexSet expected = {0, 2, 5};

      TIndexSet actual;
      sweeper.Sweep(MakeInsertFunctor(actual));

      TEST_EQUAL(expected, actual, ());
    }
  }

  {
    vector<m2::PointD> const points = {m2::PointD(0.0, 0.0), m2::PointD(1.0, 1.0),
                                       m2::PointD(1.5, 0.0), m2::PointD(1.5 + 1.01, 1.5 + 1.0)};
    NearbyPointsSweeper sweeper(1.0);

    for (size_t i = 0; i < points.size(); ++i)
      sweeper.Add(points[i].x, points[i].y, i);

    TIndexSet expected = {0, 2, 3};

    TIndexSet actual;
    sweeper.Sweep(MakeInsertFunctor(actual));

    TEST_EQUAL(expected, actual, ());
  }
}

UNIT_TEST(NearbyPointsSweeper_CornerCases)
{
  vector<m2::PointD> const points = {m2::PointD(0, 0),     m2::PointD(0, 0), m2::PointD(1, 0),
                                     m2::PointD(0, 1),     m2::PointD(1, 1), m2::PointD(1, 0),
                                     m2::PointD(0.5, 0.5), m2::PointD(0, 1)};

  {
    // In accordance with the specification, all points must be left
    // as is when epsilon is negative.
    NearbyPointsSweeper sweeper(-1.0);

    TIndexSet expected;
    for (size_t i = 0; i < points.size(); ++i)
    {
      sweeper.Add(points[i].x, points[i].y, i);
      expected.insert(i);
    }

    TIndexSet actual;
    sweeper.Sweep(MakeInsertFunctor(actual));

    TEST_EQUAL(expected, actual, ());
  }

  {
    NearbyPointsSweeper sweeper(10.0);
    for (size_t i = 0; i < points.size(); ++i)
      sweeper.Add(points[i].x, points[i].y, i);

    TIndexSet expected = {0};
    TIndexSet actual;
    sweeper.Sweep(MakeInsertFunctor(actual));

    TEST_EQUAL(expected, actual, ());
  }
}
}  // namespace
}  // namespace search
