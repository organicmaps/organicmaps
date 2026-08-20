#include "testing/testing.hpp"

#include "map/search_api.hpp"

#include "search/result.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <initializer_list>

namespace search_viewport_tests
{
using namespace search;

Result MakeResult(m2::PointD const & pt)
{
  Result res(pt, "name");
  res.SetType(Result::Type::LatLon);
  return res;
}

Results MakeResults(std::initializer_list<m2::PointD> const & points)
{
  Results results;
  for (auto const & pt : points)
    results.AddResultNoChecks(MakeResult(pt));
  return results;
}

m2::AnyRectD MakeViewport(double angle = 0.0)
{
  return m2::AnyRectD(m2::PointD(0, 0), ang::AngleD(angle), m2::RectD(-10, -5, 10, 5));
}

UNIT_TEST(ExtendViewportToNearestResult_NoResults)
{
  auto viewport = MakeViewport();
  auto const original = viewport;

  TEST(!ExtendViewportToNearestResult(Results(), viewport), ());
  TEST_EQUAL(viewport.GetGlobalRect(), original.GetGlobalRect(), ());

  Results suggests;
  suggests.AddResultNoChecks(Result("ca", "cafe"));
  TEST(!ExtendViewportToNearestResult(suggests, viewport), ());
  TEST_EQUAL(viewport.GetGlobalRect(), original.GetGlobalRect(), ());
}

UNIT_TEST(ExtendViewportToNearestResult_NearestInside)
{
  auto viewport = MakeViewport();
  auto const original = viewport;

  // The nearest result is inside the viewport, so the far one must not affect it.
  TEST(!ExtendViewportToNearestResult(MakeResults({{100, 100}, {1, 1}}), viewport), ());
  TEST_EQUAL(viewport.GetGlobalRect(), original.GetGlobalRect(), ());
}

UNIT_TEST(ExtendViewportToNearestResult_NearestOutside)
{
  auto viewport = MakeViewport();
  auto const original = viewport;

  m2::PointD const nearest(2, 20);
  m2::PointD const far(0, 100);
  TEST(ExtendViewportToNearestResult(MakeResults({far, nearest}), viewport), ());

  // The viewport is zoomed out around its center to include the nearest result with a margin,
  // and the original area stays inside.
  TEST(viewport.IsPointInside(nearest), (viewport));
  TEST(m2::AlmostEqualAbs(viewport.Center(), original.Center(), 1e-9), (viewport));
  TEST_GREATER(viewport.GetLocalRect().maxY(), 20.0, (viewport));
  TEST(viewport.IsRectInside(original), (viewport));
  // The viewport is not extended to include all the results.
  TEST(!viewport.IsPointInside(far), (viewport));
}

UNIT_TEST(ExtendViewportToNearestResult_KeepsRotation)
{
  double const angle = math::pi / 4;
  auto viewport = MakeViewport(angle);
  auto const original = viewport;

  m2::PointD const nearest(20, 20);
  TEST(ExtendViewportToNearestResult(MakeResults({nearest}), viewport), ());

  TEST_ALMOST_EQUAL_ABS(viewport.Angle().val(), angle, 1e-9, ());
  TEST(viewport.IsPointInside(nearest), (viewport));
  TEST(m2::AlmostEqualAbs(viewport.Center(), original.Center(), 1e-9), (viewport));
  TEST(viewport.IsRectInside(original), (viewport));
}
}  // namespace search_viewport_tests
