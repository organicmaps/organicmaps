#include "testing/testing.hpp"

#include "search/point_rect_matcher.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <initializer_list>
#include <limits>
#include <utility>
#include <vector>

using namespace search;
using namespace std;

using PointIdPair = PointRectMatcher::PointIdPair;
using RectIdPair = PointRectMatcher::RectIdPair;

namespace
{
auto const kInvalidId = numeric_limits<size_t>::max();

UNIT_TEST(PointRectMatcher_Smoke)
{
  PointRectMatcher matcher;
  for (auto requestType : {PointRectMatcher::RequestType::Any, PointRectMatcher::RequestType::All})
  {
    matcher.Match(vector<PointIdPair>{}, vector<RectIdPair>{}, requestType,
                  [](size_t /* pointId */, size_t /* rectId */)
    { TEST(false, ("This callback should not be called!")); });
  }
}

UNIT_TEST(PointRectMatcher_Simple)
{
  vector<PointIdPair> const points = {
      {PointIdPair(m2::PointD(0, 0), 0 /* id */), PointIdPair(m2::PointD(4, 5), 1 /* id */),
       PointIdPair(m2::PointD(-10, 3), 2 /* id */), PointIdPair(m2::PointD(-10, -10), 3 /* id */)}};

  vector<RectIdPair> const rects = {{RectIdPair(m2::RectD(-1, -1, 1, 1), 0 /* id */),
                                     RectIdPair(m2::RectD(-9, -9, -8, 8), 1 /* id */),
                                     RectIdPair(m2::RectD(-11, 2, 1000, 1000), 2 /* id */)}};

  PointRectMatcher matcher;

  {
    matcher.Match(points, vector<RectIdPair>{}, PointRectMatcher::RequestType::Any, [](size_t pointId, size_t rectId)
    { TEST(false, ("Callback should not be called:", pointId, rectId)); });
  }

  {
    matcher.Match(vector<PointIdPair>{}, rects, PointRectMatcher::RequestType::Any, [](size_t pointId, size_t rectId)
    { TEST(false, ("Callback should not be called:", pointId, rectId)); });
  }

  {
    vector<size_t> actualMatching(points.size(), kInvalidId);
    matcher.Match(points, rects, PointRectMatcher::RequestType::Any, [&](size_t pointId, size_t rectId)
    {
      TEST_LESS(pointId, actualMatching.size(), ());
      TEST_EQUAL(actualMatching[pointId], kInvalidId, ());
      actualMatching[pointId] = rectId;
    });

    vector<size_t> const expectedMatching = {{0, 2, 2, kInvalidId}};
    TEST_EQUAL(actualMatching, expectedMatching, ());
  }
}

UNIT_TEST(PointRectMatcher_MultiplePointsInRect)
{
  vector<PointIdPair> const points = {
      {PointIdPair(m2::PointD(1, 1), 0 /* id */), PointIdPair(m2::PointD(2, 2), 1 /* id */),
       PointIdPair(m2::PointD(11, 1), 2 /* id */), PointIdPair(m2::PointD(12, 2), 3 /* id */),
       PointIdPair(m2::PointD(7, 1), 4 /* id */), PointIdPair(m2::PointD(8, 2), 5 /* id */)}};

  vector<RectIdPair> const rects = {{RectIdPair(m2::RectD(0, 0, 5, 5), 0 /* id */),
                                     RectIdPair(m2::RectD(10, 0, 15, 5), 1 /* id */),
                                     RectIdPair(m2::RectD(0, 0, 1000, 1000), 2 /* id */)}};

  PointRectMatcher matcher;

  {
    vector<pair<size_t, size_t>> actualCalls;
    matcher.Match(points, rects, PointRectMatcher::RequestType::All, [&](size_t pointId, size_t rectId)
    {
      TEST_LESS(pointId, points.size(), ());
      TEST_LESS(rectId, rects.size(), ());
      actualCalls.emplace_back(pointId, rectId);
    });

    vector<pair<size_t, size_t>> const expectedCalls = {{0, 0}, {0, 2}, {1, 0}, {1, 2}, {2, 1},
                                                        {2, 2}, {3, 1}, {3, 2}, {4, 2}, {5, 2}};
    sort(actualCalls.begin(), actualCalls.end());
    CHECK(is_sorted(expectedCalls.begin(), expectedCalls.end()), ());
    TEST_EQUAL(actualCalls, expectedCalls, ());
  }
}
}  // namespace
