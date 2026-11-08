#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/my_position_controller.hpp"

#include "geometry/screenbase.hpp"

namespace my_position_controller_tests
{
using df::test_support::VisualParamsFixture;

UNIT_CLASS_TEST(VisualParamsFixture, StopFollowModesAndFirstFix)
{
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-100, -100, 100, 100)));
  location::GpsInfo fix;
  fix.m_latitude = 47;
  fix.m_longitude = 8;
  fix.m_horizontalAccuracy = 5;
  fix.m_timestamp = 1;

  for (auto mode : {location::Follow, location::FollowAndRotate, location::NotFollow})
  {
    df::MyPositionController controller(
        {mode, 0.0 /* timeInBackground */, {}, false /* isRoutingActive */, false /* isAutozoomEnabled */, {}},
        nullptr);
    controller.OnUpdateScreen(screen);
    controller.OnLocationUpdate(fix, false /* isNavigable */, screen);
    TEST_EQUAL(controller.GetCurrentMode(), mode, ());
    controller.StopLocationFollow();
    TEST_EQUAL(controller.GetCurrentMode(), location::NotFollow, ());
  }

  df::MyPositionController pending({location::Follow,
                                    0.0 /* timeInBackground */,
                                    {},
                                    false /* isRoutingActive */,
                                    false /* isAutozoomEnabled */,
                                    {}},
                                   nullptr);
  pending.OnUpdateScreen(screen);
  TEST_EQUAL(pending.GetCurrentMode(), location::PendingPosition, ());
  pending.StopLocationFollow();
  pending.OnLocationUpdate(fix, false /* isNavigable */, screen);
  TEST_EQUAL(pending.GetCurrentMode(), location::NotFollow, ());
}

UNIT_CLASS_TEST(VisualParamsFixture, HistoricalPositionIsDisplayedObsolete)
{
  using namespace std::chrono_literals;
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-100, -100, 100, 100)));
  df::MyPositionController controller({location::NotFollow, 0.0, {}, false, false, {}}, nullptr);
  controller.OnUpdateScreen(screen);

  location::GpsInfo fix;
  fix.m_source = location::EAndroidNative;
  fix.m_latitude = 47;
  fix.m_longitude = 8;
  fix.m_horizontalAccuracy = 5;
  fix.m_timestamp = 1000;
  auto const old = location::Clock::now() - 31s;
  fix.m_timing = location::GpsInfo::Timing{old, old + 5s};
  fix.m_speed = 14;
  fix.m_bearing = 90;
  controller.OnLocationUpdate(fix, false, screen);

  TEST(controller.IsModeHasPosition(), ());
  TEST(controller.IsPositionObsolete(), ());
  TEST(!controller.IsRotationAvailable(), ());
}

UNIT_CLASS_TEST(VisualParamsFixture, DisplayObsolescenceKeepsThirtySecondLifetime)
{
  using namespace std::chrono_literals;
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-100, -100, 100, 100)));
  df::MyPositionController controller({location::NotFollow, 0.0, {}, false, false, {}}, nullptr);
  controller.OnUpdateScreen(screen);

  location::GpsInfo fix;
  fix.m_source = location::EAndroidNative;
  fix.m_latitude = 47;
  fix.m_longitude = 8;
  fix.m_timestamp = 1000;
  auto const old = location::Clock::now() - 10s;
  fix.m_timing = location::GpsInfo::Timing{old, old + 5s};
  controller.OnLocationUpdate(fix, false, screen);

  TEST(!fix.IsFresh(), ());
  TEST(!controller.IsPositionObsolete(), ());
}
}  // namespace my_position_controller_tests
