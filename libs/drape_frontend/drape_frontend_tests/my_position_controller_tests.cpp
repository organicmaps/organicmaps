#include "testing/testing.hpp"

#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/my_position_controller.hpp"

#include "geometry/mercator.hpp"
#include "geometry/screenbase.hpp"

#include <chrono>

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

  df::AnimationSystem animations;
  for (auto mode : {location::Follow, location::FollowAndRotate, location::NotFollow})
  {
    df::MyPositionController controller(
        {mode, 0.0 /* timeInBackground */, {}, false /* isRoutingActive */, false /* isAutozoomEnabled */, {}}, nullptr,
        animations);
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
                                   nullptr, animations);
  pending.OnUpdateScreen(screen);
  TEST_EQUAL(pending.GetCurrentMode(), location::PendingPosition, ());
  pending.StopLocationFollow();
  pending.OnLocationUpdate(fix, false /* isNavigable */, screen);
  TEST_EQUAL(pending.GetCurrentMode(), location::NotFollow, ());
}

UNIT_CLASS_TEST(VisualParamsFixture, ArrowAnimatesWithinTheTimeLimit)
{
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-2.0, -2.0, 2.0, 2.0)));
  df::AnimationSystem animations;
  df::MyPositionController controller({location::NotFollow, 0.0, {}, false, false, {}}, nullptr, animations);
  controller.OnUpdateScreen(screen);

  location::GpsInfo fix;
  fix.m_horizontalAccuracy = 5.0;
  fix.m_timestamp = 1.0;
  controller.OnLocationUpdate(fix, false, screen);
  auto const start = controller.Position();

  // A short move starts at the displayed pose and takes kShortMoveTimeSec. The controller reads the clock itself, so
  // the test checks only the ends of the move.
  auto const duration =
      std::chrono::duration_cast<df::AnimationClock::duration>(std::chrono::duration<double>(df::kShortMoveTimeSec));
  fix.m_longitude = mercator::XToLon(start.x + 0.3);
  fix.m_timestamp += 1.0;
  controller.OnLocationUpdate(fix, false, screen);
  auto const moving = animations.GetArrowPose();
  TEST(moving.has_value(), ());
  TEST_EQUAL(moving->m_position, start, ());
  animations.Advance(df::AnimationClock::now() + duration);
  TEST(controller.GetDrawablePosition().EqualDxDy(controller.Position(), 1e-9), ());

  // A move that needs more than kMaxAnimationTimeSec snaps the arrow.
  fix.m_longitude = mercator::XToLon(start.x + 150.0);
  fix.m_timestamp += 1.0;
  controller.OnLocationUpdate(fix, false, screen);
  TEST(!animations.GetArrowPose().has_value(), ());
  TEST(controller.GetDrawablePosition().EqualDxDy(controller.Position(), 1e-9), ());
}
}  // namespace my_position_controller_tests
