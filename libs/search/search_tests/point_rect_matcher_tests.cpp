#include "testing/testing.hpp"

#include "search/point_rect_matcher.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <initializer_list>
#include <limits>
#include <utility>
#include <vector>

using PointIdPair = search::PointRectMatcher::PointIdPair;
using RectIdPair = search::PointRectMatcher::RectIdPair;

namespace
{
auto const kInvalidId = std::numeric_limits<size_t>::max();

UNIT_TEST(PointRectMatcher_Smoke)
{
  search::PointRectMatcher matcher;
  for (auto requestType : {search::PointRectMatcher::RequestType::Any, search::PointRectMatcher::RequestType::All})
  {
    matcher.Match(std::vector<PointIdPair>{}, std::vector<RectIdPair>{}, requestType,
                  [](size_t /* pointId */, size_t /* rectId */)
    { TEST(false, ("This callback should not be called!")); });
  }
}

UNIT_TEST(PointRectMatcher_Simple)
{
  std::vector<PointIdPair> const points = {
      {PointIdPair(m2::PointD(0, 0), 0 /* id */), PointIdPair(m2::PointD(4, 5), 1 /* id */),
       PointIdPair(m2::PointD(-10, 3), 2 /* id */), PointIdPair(m2::PointD(-10, -10), 3 /* id */)}};

  std::vector<RectIdPair> const rects = {{RectIdPair(m2::RectD(-1, -1, 1, 1), 0 /* id */),
                                          RectIdPair(m2::RectD(-9, -9, -8, 8), 1 /* id */),
                                          RectIdPair(m2::RectD(-11, 2, 1000, 1000), 2 /* id */)}};

  search::PointRectMatcher matcher;

  {
    matcher.Match(points, std::vector<RectIdPair>{}, search::PointRectMatcher::RequestType::Any,
                  [](size_t pointId, size_t rectId)
    { TEST(false, ("Callback should not be called:", pointId, rectId)); });
  }

  {
    matcher.Match(std::vector<PointIdPair>{}, rects, search::PointRectMatcher::RequestType::Any,
                  [](size_t pointId, size_t rectId)
    { TEST(false, ("Callback should not be called:", pointId, rectId)); });
  }

  {
    std::vector<size_t> actualMatching(points.size(), kInvalidId);
    matcher.Match(points, rects, search::PointRectMatcher::RequestType::Any, [&](size_t pointId, size_t rectId)
    {
      TEST_LESS(pointId, actualMatching.size(), ());
      TEST_EQUAL(actualMatching[pointId], kInvalidId, ());
      actualMatching[pointId] = rectId;
    });

    std::vector<size_t> const expectedMatching = {{0, 2, 2, kInvalidId}};
    TEST_EQUAL(actualMatching, expectedMatching, ());
  }
}

UNIT_TEST(PointRectMatcher_MultiplePointsInRect)
{
  std::vector<PointIdPair> const points = {
      {PointIdPair(m2::PointD(1, 1), 0 /* id */), PointIdPair(m2::PointD(2, 2), 1 /* id */),
       PointIdPair(m2::PointD(11, 1), 2 /* id */), PointIdPair(m2::PointD(12, 2), 3 /* id */),
       PointIdPair(m2::PointD(7, 1), 4 /* id */), PointIdPair(m2::PointD(8, 2), 5 /* id */)}};

  std::vector<RectIdPair> const rects = {{RectIdPair(m2::RectD(0, 0, 5, 5), 0 /* id */),
                                          RectIdPair(m2::RectD(10, 0, 15, 5), 1 /* id */),
                                          RectIdPair(m2::RectD(0, 0, 1000, 1000), 2 /* id */)}};

  search::PointRectMatcher matcher;

  {
    std::vector<std::pair<size_t, size_t>> actualCalls;
    matcher.Match(points, rects, search::PointRectMatcher::RequestType::All, [&](size_t pointId, size_t rectId)
    {
      TEST_LESS(pointId, points.size(), ());
      TEST_LESS(rectId, rects.size(), ());
      actualCalls.emplace_back(pointId, rectId);
    });

    std::vector<std::pair<size_t, size_t>> const expectedCalls = {{0, 0}, {0, 2}, {1, 0}, {1, 2}, {2, 1},
                                                                  {2, 2}, {3, 1}, {3, 2}, {4, 2}, {5, 2}};
    std::sort(actualCalls.begin(), actualCalls.end());
    CHECK(std::is_sorted(expectedCalls.begin(), expectedCalls.end()), ());
    TEST_EQUAL(actualCalls, expectedCalls, ());
  }
}
}  // namespace
