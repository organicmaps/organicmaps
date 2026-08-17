#include "testing/testing.hpp"

#include "indexer/feature_covering.hpp"
#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

namespace feature_covering_test
{
using namespace covering;
using namespace mercator;

Intervals GetterIntervals(m2::RectD const & r, int scale)
{
  CoveringGetter g(r, ViewportWithLowLevels);
  return g.Get(scale);
}

int64_t TotalLength(Intervals const & v)
{
  int64_t len = 0;
  for (auto const & i : v)
    len += i.second - i.first;
  return len;
}

void TestSortedAndDisjoint(Intervals const & v)
{
  for (size_t i = 0; i < v.size(); ++i)
  {
    TEST_LESS(v[i].first, v[i].second, (i, v));
    // After merging, intervals are strictly ordered and non-touching.
    if (i > 0)
      TEST_LESS(v[i - 1].second, v[i].first, (i, v));
  }
}

// A one-rect Covering must produce exactly the same intervals as CoveringGetter for that rect,
// so existing single-rect queries can migrate to Covering without any behaviour change.
UNIT_TEST(Covering_SingleRectEqualsCoveringGetter)
{
  auto const r = RectByCenterXYAndSizeInMeters(FromLatLon(55.75, 37.62), 100.0);
  // Equivalence must hold at any coding scale (deepest and a shallower one).
  for (int const scale : {scales::GetUpperScale(), scales::GetUpperScale() - 3})
  {
    AggCovering cov(scale);
    cov.Add(r);
    TEST_EQUAL(cov.Get(scale), GetterIntervals(r, scale), (scale));
  }
}

UNIT_TEST(Covering_UnionOfRects)
{
  auto const r1 = RectByCenterXYAndSizeInMeters(FromLatLon(55.75, 37.62), 100.0);
  auto const r2 = RectByCenterXYAndSizeInMeters(FromLatLon(59.93, 30.33), 100.0);
  int const scale = scales::GetUpperScale();

  AggCovering cov(scale);
  cov.Add(r1);
  cov.Add(r2);
  auto const & res = cov.Get(scale);

  // The aggregated covering is the merge of the two individual coverings.
  Intervals all = GetterIntervals(r1, scale);
  Intervals const second = GetterIntervals(r2, scale);
  all.insert(all.end(), second.begin(), second.end());
  TEST_EQUAL(res, SortAndMergeIntervals(all), ());

  TestSortedAndDisjoint(res);

  // The bounding rect spans both added rects.
  m2::RectD unionRect = r1;
  unionRect.Add(r2);
  TEST_EQUAL(cov.GetRect(), unionRect, ());
}

// The point of Covering: aggregating two far-apart small rects reads far fewer index cells than
// covering their (huge) bounding rect would.
UNIT_TEST(Covering_AggregatedIsTighterThanBoundingRect)
{
  auto const r1 = RectByCenterXYAndSizeInMeters(FromLatLon(55.75, 37.62), 100.0);
  auto const r2 = RectByCenterXYAndSizeInMeters(FromLatLon(59.93, 30.33), 100.0);
  int const scale = scales::GetUpperScale();

  AggCovering cov(scale);
  cov.Add(r1);
  cov.Add(r2);

  m2::RectD bbox = r1;
  bbox.Add(r2);

  TEST_LESS(TotalLength(cov.Get(scale)), TotalLength(GetterIntervals(bbox, scale)), ());
}

UNIT_TEST(Covering_Empty)
{
  int const scale = scales::GetUpperScale();
  AggCovering cov(scale);
  TEST(cov.IsEmpty(), ());
  TEST(cov.Get(scale).empty(), ());
}
}  // namespace feature_covering_test
