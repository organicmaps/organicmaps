#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/my_position_controller.hpp"

#include "geometry/mercator.hpp"
#include "geometry/screenbase.hpp"

namespace my_position_controller_tests
{
using df::test_support::VisualParamsFixture;

namespace
{
ScreenBase MakeScreen()
{
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-100, -100, 100, 100)));
  return screen;
}

location::GpsInfo MakeFix()
{
  location::GpsInfo fix;
  fix.m_latitude = 47;
  fix.m_longitude = 8;
  fix.m_horizontalAccuracy = 5;
  fix.m_timestamp = 1;
  return fix;
}

df::MyPositionController::Params MakeParams(location::EMyPositionMode mode)
{
  return {mode, 0.0 /* timeInBackground */, {}, false /* isRoutingActive */, false /* isAutozoomEnabled */, {}};
}
}  // namespace

UNIT_CLASS_TEST(VisualParamsFixture, StopFollowModesAndFirstFix)
{
  ScreenBase const screen = MakeScreen();
  location::GpsInfo const fix = MakeFix();

  for (auto mode : {location::Follow, location::FollowAndRotate, location::NotFollow})
  {
    df::MyPositionController controller(MakeParams(mode), nullptr);
    controller.OnUpdateScreen(screen);
    controller.OnLocationUpdate(fix, false /* isNavigable */, screen);
    TEST_EQUAL(controller.GetCurrentMode(), mode, ());
    controller.StopLocationFollow();
    TEST_EQUAL(controller.GetCurrentMode(), location::NotFollow, ());
  }

  df::MyPositionController pending(MakeParams(location::Follow), nullptr);
  pending.OnUpdateScreen(screen);
  TEST_EQUAL(pending.GetCurrentMode(), location::PendingPosition, ());
  pending.StopLocationFollow();
  pending.OnLocationUpdate(fix, false /* isNavigable */, screen);
  TEST_EQUAL(pending.GetCurrentMode(), location::NotFollow, ());
}

UNIT_CLASS_TEST(VisualParamsFixture, HiddenMarkKeepsPositionAndMode)
{
  ScreenBase const screen = MakeScreen();
  location::GpsInfo const fix = MakeFix();

  df::MyPositionController controller(MakeParams(location::Follow), nullptr);
  controller.OnUpdateScreen(screen);
  controller.SetVisible(false);
  controller.OnLocationUpdate(fix, false /* isNavigable */, screen);
  TEST(!controller.IsVisible(), ());
  TEST_EQUAL(controller.GetCurrentMode(), location::Follow, ());
  TEST_EQUAL(controller.Position(), mercator::FromLatLon(fix.m_latitude, fix.m_longitude), ());
}
}  // namespace my_position_controller_tests
