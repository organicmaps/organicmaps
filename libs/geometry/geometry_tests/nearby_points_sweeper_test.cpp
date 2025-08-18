#include "testing/testing.hpp"

#include "geometry/nearby_points_sweeper.hpp"
#include "geometry/point2d.hpp"

#include "base/stl_helpers.hpp"

#include <set>
#include <utility>
#include <vector>

namespace nearby_points_sweeper_test
{
using namespace m2;
using namespace std;

// Multiset is used here to catch situations when some index is reported more than once.
using TIndexSet = multiset<size_t>;

UNIT_TEST(NearbyPointsSweeper_Smoke)
{
  {
    uint8_t const priority = 0;
    NearbyPointsSweeper sweeper(0.0);
    for (size_t i = 0; i < 10; ++i)
      sweeper.Add(10.0, 10.0, i, priority);

    TIndexSet expected = {0};

    TIndexSet actual;
    sweeper.Sweep(base::MakeInsertFunctor(actual));

    TEST_EQUAL(expected, actual, ());
  }

  {
    uint8_t const priority = 0;
    vector<double> const coords = {0.0, 0.5, 1.0, 1.5, 1.4, 1.6};

    {
      NearbyPointsSweeper sweeper(0.5);

      for (size_t i = 0; i < coords.size(); ++i)
        sweeper.Add(coords[i], 0.0, i, priority);

      TIndexSet expected = {0, 2, 5};

      TIndexSet actual;
      sweeper.Sweep(base::MakeInsertFunctor(actual));

      TEST_EQUAL(expected, actual, ());
    }

    {
      NearbyPointsSweeper sweeper(0.5);

      for (size_t i = 0; i < coords.size(); ++i)
        sweeper.Add(0.0, coords[i], i, priority);

      TIndexSet expected = {0, 2, 5};

      TIndexSet actual;
      sweeper.Sweep(base::MakeInsertFunctor(actual));

      TEST_EQUAL(expected, actual, ());
    }
  }

  {
    uint8_t const priority = 0;
    vector<PointD> const points = {PointD(0.0, 0.0), PointD(1.0, 1.0), PointD(1.5, 0.0), PointD(1.5 + 1.01, 1.5 + 1.0)};
    NearbyPointsSweeper sweeper(1.0);

    for (size_t i = 0; i < points.size(); ++i)
      sweeper.Add(points[i].x, points[i].y, i, priority);

    TIndexSet expected = {0, 2, 3};

    TIndexSet actual;
    sweeper.Sweep(base::MakeInsertFunctor(actual));

    TEST_EQUAL(expected, actual, ());
  }

  {
    uint8_t const priority = 0;
    vector<PointD> const points = {PointD(0, 0), PointD(0, 0), PointD(1, 0),     PointD(0, 1),
                                   PointD(1, 1), PointD(1, 0), PointD(0.5, 0.5), PointD(0, 1)};

    NearbyPointsSweeper sweeper(10.0);
    for (size_t i = 0; i < points.size(); ++i)
      sweeper.Add(points[i].x, points[i].y, i, priority);

    TIndexSet expected = {0};
    TIndexSet actual;
    sweeper.Sweep(base::MakeInsertFunctor(actual));

    TEST_EQUAL(expected, actual, ());
  }
}

UNIT_TEST(NearbyPointsSweeper_Priority)
{
  {
    NearbyPointsSweeper sweeper(0.0);
    for (size_t i = 0; i < 10; ++i)
      sweeper.Add(10.0, 10.0, i /* index */, i /* priority */);

    TIndexSet expected = {9};

    TIndexSet actual;
    sweeper.Sweep(base::MakeInsertFunctor(actual));

    TEST_EQUAL(expected, actual, ());
  }
  {
    vector<pair<double, uint8_t>> const objects = {{0.0, 0}, {0.5, 1}, {1.0, 1}, {1.5, 1}, {1.4, 0}, {1.6, 0}};
    NearbyPointsSweeper sweeper(0.5);

    for (size_t i = 0; i < objects.size(); ++i)
      sweeper.Add(objects[i].first, 0.0, i /* index */, objects[i].second);

    TIndexSet expected = {1, 3};

    TIndexSet actual;
    sweeper.Sweep(base::MakeInsertFunctor(actual));

    TEST_EQUAL(expected, actual, ());
  }
  {
    vector<pair<PointD, uint8_t>> const objects = {
        {PointD(0.0, 0.0), 0}, {PointD(1.0, 1.0), 1}, {PointD(1.5, 0.0), 0}, {PointD(1.5 + 1.01, 1.5 + 1.0), 0}};
    NearbyPointsSweeper sweeper(1.0);

    for (size_t i = 0; i < objects.size(); ++i)
      sweeper.Add(objects[i].first.x, objects[i].first.y, i /* index */, objects[i].second);

    TIndexSet expected = {1, 3};

    TIndexSet actual;
    sweeper.Sweep(base::MakeInsertFunctor(actual));

    TEST_EQUAL(expected, actual, ());
  }
}
}  // namespace nearby_points_sweeper_test
