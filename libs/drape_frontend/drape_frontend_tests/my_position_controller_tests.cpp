#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"

#include "drape_frontend/arrow3d.hpp"
#include "drape_frontend/drape_hints.hpp"
#include "drape_frontend/my_position_controller.hpp"
#include "drape_frontend/visual_params.hpp"

#include "platform/location.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

namespace my_position_controller_tests
{
using df::test_support::VisualParamsFixture;

// Other tests in this binary rely on the visual scale set by VisualParamsFixture.
class ScopedVisualScale
{
public:
  explicit ScopedVisualScale(double vs) : m_prev(df::VisualParams::Instance().GetVisualScale())
  {
    df::VisualParams::Instance().SetVisualScale(vs);
  }
  ~ScopedVisualScale() { df::VisualParams::Instance().SetVisualScale(m_prev); }

private:
  double const m_prev;
};

m2::RectD const kViewport(0.0, 0.0, 600.0, 800.0);

// Returns the bottom offset of the position anchor in routing mode, in device pixels.
double GetRoutingOffsetY(df::MyPositionController & controller)
{
  m2::PointD anchor = m2::PointD::Zero();
  controller.CorrectScalePoint(anchor);
  return kViewport.maxY() - anchor.y;
}

// The routing anchor must follow the visual scale, which changes at runtime when the map is moved
// to a display with another scale (CarPlay/Android Auto connect and disconnect).
UNIT_CLASS_TEST(VisualParamsFixture, MyPositionController_RoutingOffsetFollowsVisualScale)
{
  df::MyPositionController::Params params(location::FollowAndRotate, 0.0 /* timeInBackground */, df::Hints{},
                                          true /* isRoutingActive */, false /* isAutozoomEnabled */,
                                          [](location::EMyPositionMode, bool) {});
  df::MyPositionController controller(std::move(params), nullptr /* notifier */);
  controller.SetVisibleViewport(kViewport);

  // A position is needed to leave PendingPosition for FollowAndRotate, where the routing anchor is used.
  location::GpsInfo info;
  info.m_latitude = 0.0;
  info.m_longitude = 0.0;
  info.m_horizontalAccuracy = 10.0;
  controller.OnLocationUpdate(info, true /* isNavigable */, ScreenBase());
  TEST(controller.GetCurrentMode() == location::FollowAndRotate, (controller.GetCurrentMode()));

  double const visualScale = df::VisualParams::Instance().GetVisualScale();
  double constexpr kChangedVisualScale = 3.0;

  // The default offset is fully derived from the visual scale.
  double const defaultOffset = GetRoutingOffsetY(controller);
  TEST_GREATER(defaultOffset, 0.0, ());
  {
    ScopedVisualScale const guard(kChangedVisualScale);
    TEST_ALMOST_EQUAL_ABS(GetRoutingOffsetY(controller), defaultOffset * kChangedVisualScale / visualScale, 1e-9, ());
  }
  TEST_ALMOST_EQUAL_ABS(GetRoutingOffsetY(controller), defaultOffset, 1e-9, ());

  // An offset reported by the UI is in device pixels and is not scaled, but the arrow's size on top of it is.
  int constexpr kUiOffset = 5;
  controller.UpdateRoutingOffsetY(false /* useDefault */, kUiOffset);
  double const uiOffset = GetRoutingOffsetY(controller);
  TEST_ALMOST_EQUAL_ABS(uiOffset, kUiOffset + df::Arrow3d::GetMaxBottomSize() * visualScale, 1e-9, ());
  {
    ScopedVisualScale const guard(kChangedVisualScale);
    TEST_ALMOST_EQUAL_ABS(GetRoutingOffsetY(controller),
                          kUiOffset + df::Arrow3d::GetMaxBottomSize() * kChangedVisualScale, 1e-9, ());
  }

  controller.UpdateRoutingOffsetY(true /* useDefault */, 0 /* offsetY */);
  TEST_ALMOST_EQUAL_ABS(GetRoutingOffsetY(controller), defaultOffset, 1e-9, ());
}
}  // namespace my_position_controller_tests
