#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/frontend_renderer.hpp"

#include <algorithm>

namespace bookmark_scaling_tests
{
using df::test_support::VisualParamsFixture;

UNIT_CLASS_TEST(VisualParamsFixture, BookmarkTapTargetScalesAroundPinTip)
{
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-400, -300, 400, 300)));
  m2::PointD const tap(0, 0);
  auto const baseline = df::TapInfo::GetBookmarkTapRect(tap, screen).GetGlobalRect();

  for (double const scale : {0.8, 1.0, 1.4, 1.8, 2.0})
  {
    auto const rect = df::TapInfo::GetBookmarkTapRect(tap, screen, scale).GetGlobalRect();
    double const expectedScale = std::max(1.0, scale);
    m2::RectD const expected(baseline.LeftBottom() * expectedScale, baseline.RightTop() * expectedScale);
    TEST(m2::AlmostEqualAbs(rect, expected, 1e-6), (scale, rect, expected));
  }
}
}  // namespace bookmark_scaling_tests
