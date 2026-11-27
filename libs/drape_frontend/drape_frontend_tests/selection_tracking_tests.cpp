#include "testing/testing.hpp"

#include "drape_frontend/selection_tracking.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

namespace selection_tracking_tests
{
m2::RectD const kScreen(0, 0, 1080, 2400);
double constexpr kHalfSize = 170;

// Moves the selection by the offset the map is scrolled by.
m2::PointD ChangeViewport(df::SelectionTrackInfo & info, m2::RectD const & oldViewport, m2::RectD const & newViewport,
                          m2::PointD & pos)
{
  auto const offset = info.CalcOffset(oldViewport, newViewport, m2::RectD(pos, kHalfSize, kHalfSize), pos);
  pos += offset;
  return offset;
}

UNIT_TEST(SelectionTracking_VisibleSelectionStays)
{
  m2::PointD pos(540, 500);
  df::SelectionTrackInfo info;
  auto const offset = ChangeViewport(info, kScreen, {0, 0, 1080, 1100}, pos);
  TEST_EQUAL(offset, m2::PointD::Zero(), ());
  TEST_EQUAL(info.m_snapSides, m2::PointI::Zero(), ());
}

UNIT_TEST(SelectionTracking_PanelExpandsInStepsAndFalls)
{
  m2::PointD const start(540, 1000);
  auto pos = start;
  df::SelectionTrackInfo info;
  m2::RectD const collapsed(0, 0, 1080, 1100);
  m2::RectD const expanded(0, 0, 1080, 800);

  auto const collapse = ChangeViewport(info, kScreen, collapsed, pos);
  TEST_EQUAL(collapse, m2::PointD(0, -70), ());
  auto const expand = ChangeViewport(info, collapsed, expanded, pos);
  TEST_EQUAL(expand, m2::PointD(0, -300), ());
  auto const fall = ChangeViewport(info, expanded, kScreen, pos);
  TEST_EQUAL(fall, m2::PointD(0, 370), ());
  TEST_EQUAL(pos, start, ());
}

// A selection near a corner is scrolled on both axes. When the panel falls, only the vertical correction is rolled
// back: rolling back the horizontal one would hide the right edge of the selection again.
UNIT_TEST(SelectionTracking_NearCornerScrollsBothAxes)
{
  m2::PointD pos(1000, 1000);
  df::SelectionTrackInfo info;
  m2::RectD const panelUp(0, 0, 1080, 1100);

  auto const rise = ChangeViewport(info, kScreen, panelUp, pos);
  TEST_EQUAL(rise, m2::PointD(-90, -70), ());
  auto const fall = ChangeViewport(info, panelUp, kScreen, pos);
  TEST_EQUAL(fall, m2::PointD(0, 70), ());
  TEST_EQUAL(pos, m2::PointD(910, 1000), ());
}
}  // namespace selection_tracking_tests
