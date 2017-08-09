#include "testing/testing.hpp"

#include "search/point_rect_matcher.hpp"

#include <limits>
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
  matcher.Match(vector<PointIdPair>{}, vector<RectIdPair>{},
                [](size_t /* pointId */, size_t /* rectId */) {
                  TEST(false, ("This callback should not be called!"));
                });
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
    matcher.Match(points, vector<RectIdPair>{}, [](size_t pointId, size_t rectId) {
      TEST(false, ("Callback should not be called:", pointId, rectId));
    });
  }

  {
    matcher.Match(vector<PointIdPair>{}, rects, [](size_t pointId, size_t rectId) {
      TEST(false, ("Callback should not be called:", pointId, rectId));
    });
  }

  {
    vector<size_t> actualMatching(points.size(), kInvalidId);
    matcher.Match(points, rects, [&](size_t pointId, size_t rectId) {
      TEST_LESS(pointId, actualMatching.size(), ());
      TEST_EQUAL(actualMatching[pointId], kInvalidId, ());
      actualMatching[pointId] = rectId;
    });

    vector<size_t> const expectedMatching = {{0, 2, 2, kInvalidId}};
    TEST_EQUAL(actualMatching, expectedMatching, ());
  }
}
}  // namespace
