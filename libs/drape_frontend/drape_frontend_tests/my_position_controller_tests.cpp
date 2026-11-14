#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/my_position_controller.hpp"

#include "geometry/screenbase.hpp"

namespace my_position_controller_tests
{
using df::test_support::VisualParamsFixture;

namespace
{
class PositionListener : public df::MyPositionController::Listener
{
public:
  void PositionChanged(m2::PointD const &, bool) override { ++m_positionUpdates; }
  void ChangeModelView(m2::PointD const & center, int, df::TAnimationCreator const &) override
  {
    m_lastCenter = center;
    ++m_viewportChanges;
  }
  void ChangeModelView(double, df::TAnimationCreator const &) override
  {
    ++m_viewportChanges;
    ++m_rotations;
  }
  void ChangeModelView(m2::RectD const &, df::TAnimationCreator const &) override { ++m_viewportChanges; }
  void ChangeModelView(m2::PointD const & center, double, m2::PointD const &, int, df::Animation::TAction const &,
                       df::TAnimationCreator const &) override
  {
    m_lastCenter = center;
    ++m_viewportChanges;
  }
  void ChangeModelView(double, m2::PointD const & center, double, m2::PointD const &,
                       df::TAnimationCreator const &) override
  {
    m_lastCenter = center;
    ++m_viewportChanges;
  }

  size_t m_positionUpdates = 0;
  size_t m_viewportChanges = 0;
  size_t m_rotations = 0;
  m2::PointD m_lastCenter;
};

ScreenBase MakeScreen()
{
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-100, -100, 100, 100)));
  return screen;
}

location::GpsInfo MakeLocation()
{
  location::GpsInfo fix;
  fix.m_latitude = 47;
  fix.m_longitude = 8;
  fix.m_horizontalAccuracy = 5;
  fix.m_timestamp = 1;
  return fix;
}
}  // namespace

UNIT_CLASS_TEST(VisualParamsFixture, StopFollowModesAndFirstFix)
{
  auto const screen = MakeScreen();
  auto const fix = MakeLocation();

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

UNIT_CLASS_TEST(VisualParamsFixture, PauseFollowKeepsModeAcrossUpdatesAndRepeatedTaps)
{
  auto const screen = MakeScreen();
  for (bool const routing : {false, true})
  {
    for (auto const mode : {location::Follow, location::FollowAndRotate})
    {
      size_t modeChanges = 0;
      PositionListener listener;
      df::MyPositionController controller(
          {mode, 0.0, {}, routing, false, [&modeChanges](location::EMyPositionMode, bool) { ++modeChanges; }}, nullptr);
      controller.SetListener(make_ref(&listener));
      controller.OnUpdateScreen(screen);
      auto fix = MakeLocation();
      controller.OnLocationUpdate(fix, routing, screen);
      listener.m_viewportChanges = 0;
      auto const previousModeChanges = modeChanges;

      controller.SetLocationFollowPaused(true);
      TEST_EQUAL(controller.GetCurrentMode(), mode, ());
      TEST(!controller.IsModeChangeViewport(), ());
      TEST(!controller.IsRouteFollowingActive(), ());

      m2::PointD scalePoint(100, 100);
      controller.CorrectScalePoint(scalePoint);
      TEST_EQUAL(scalePoint, m2::PointD(100, 100), ());
      controller.CorrectGlobalScalePoint(scalePoint);
      TEST_EQUAL(scalePoint, m2::PointD(100, 100), ());

      controller.SetVisibleViewport({0, 0, 800, 300});
      controller.UpdatePosition();

      fix.m_latitude += 0.001;
      ++fix.m_timestamp;
      controller.OnLocationUpdate(fix, routing, screen);
      controller.OnCompassUpdate(location::CompassInfo(1.0), screen);
      controller.UpdatePosition();
      // Selecting another point must keep the same pause and mode.
      controller.SetLocationFollowPaused(true);
      TEST_EQUAL(controller.GetCurrentMode(), mode, ());
      TEST_EQUAL(modeChanges, previousModeChanges, ());
      TEST_EQUAL(listener.m_positionUpdates, 2, ());
      TEST_EQUAL(listener.m_viewportChanges, 0, ());

      controller.SetLocationFollowPaused(false);
      TEST_EQUAL(controller.GetCurrentMode(), mode, ());
      TEST(controller.IsModeChangeViewport(), ());
      TEST_EQUAL(controller.IsRouteFollowingActive(), routing && mode == location::FollowAndRotate, ());
      TEST_EQUAL(modeChanges, previousModeChanges, ());
      TEST_EQUAL(listener.m_viewportChanges, 1, ());
      TEST_EQUAL(listener.m_lastCenter, controller.Position(), ());
      controller.SetLocationFollowPaused(false);
      TEST_EQUAL(listener.m_viewportChanges, 1, ());

      controller.DragStarted();
      controller.DragEnded({400, 300});
      TEST_EQUAL(controller.GetCurrentMode(), location::NotFollow, ());
    }
  }
}

UNIT_CLASS_TEST(VisualParamsFixture, PauseFollowManualGesturesCancelFollow)
{
  enum class Gesture
  {
    Drag,
    Pinch,
    Rotate,
    Scroll
  };

  auto const screen = MakeScreen();
  for (bool const routing : {false, true})
  {
    for (auto const mode : {location::Follow, location::FollowAndRotate})
    {
      for (auto const gesture : {Gesture::Drag, Gesture::Pinch, Gesture::Rotate, Gesture::Scroll})
      {
        PositionListener listener;
        df::MyPositionController controller({mode, 0.0, {}, routing, false, {}}, nullptr);
        controller.SetListener(make_ref(&listener));
        controller.OnUpdateScreen(screen);
        controller.OnLocationUpdate(MakeLocation(), routing, screen);
        controller.SetLocationFollowPaused(true);
        listener.m_viewportChanges = 0;

        switch (gesture)
        {
        case Gesture::Drag:
          controller.DragStarted();
          controller.DragEnded({10, 0});
          break;
        case Gesture::Pinch:
        case Gesture::Rotate:
          controller.ScaleStarted();
          if (gesture == Gesture::Rotate)
            controller.Rotated();
          controller.ScaleEnded();
          break;
        case Gesture::Scroll: controller.Scrolled({10, 0}); break;
        }

        TEST_EQUAL(controller.GetCurrentMode(), location::NotFollow, ());
        controller.SetLocationFollowPaused(true);
        // Closing the Place Page after a manual gesture must not restore following.
        controller.SetLocationFollowPaused(false);
        controller.UpdatePosition();
        TEST_EQUAL(controller.GetCurrentMode(), location::NotFollow, ());
        TEST_EQUAL(listener.m_viewportChanges, 0, ());
      }
    }
  }
}

UNIT_CLASS_TEST(VisualParamsFixture, PauseFollowDoesNotChangeFirstFixOrInactiveModes)
{
  auto const screen = MakeScreen();
  for (auto const mode :
       {location::Follow, location::FollowAndRotate, location::NotFollow, location::NotFollowNoPosition})
  {
    PositionListener listener;
    df::MyPositionController controller({mode, 0.0, {}, false, false, {}}, nullptr);
    controller.SetListener(make_ref(&listener));
    controller.OnUpdateScreen(screen);
    controller.SetLocationFollowPaused(true);
    controller.OnLocationUpdate(MakeLocation(), false, screen);
    TEST_EQUAL(controller.GetCurrentMode(), mode, ());
    TEST_EQUAL(controller.IsModeChangeViewport(), mode == location::Follow || mode == location::FollowAndRotate, ());
    auto const previousViewportChanges = listener.m_viewportChanges;
    controller.SetLocationFollowPaused(false);
    TEST_EQUAL(listener.m_viewportChanges, previousViewportChanges, ());
  }
}

UNIT_CLASS_TEST(VisualParamsFixture, PauseFollowLocationButtonSwitchesMode)
{
  auto const screen = MakeScreen();
  for (bool const routing : {false, true})
  {
    for (auto const mode : {location::Follow, location::FollowAndRotate})
    {
      PositionListener listener;
      df::MyPositionController controller({mode, 0.0, {}, routing, false, {}}, nullptr);
      controller.SetListener(make_ref(&listener));
      controller.OnUpdateScreen(screen);
      controller.OnLocationUpdate(MakeLocation(), routing, screen);
      controller.OnCompassUpdate(location::CompassInfo(1.0), screen);
      controller.SetLocationFollowPaused(true);
      listener.m_viewportChanges = 0;

      controller.NextMode(screen);
      TEST_EQUAL(controller.GetCurrentMode(), mode == location::Follow ? location::FollowAndRotate : location::Follow,
                 ());
      TEST(controller.IsModeChangeViewport(), ());
      TEST_EQUAL(listener.m_viewportChanges, 1, ());
      controller.SetLocationFollowPaused(false);
      TEST_EQUAL(listener.m_viewportChanges, 1, ());
    }
  }
}

UNIT_CLASS_TEST(VisualParamsFixture, PauseFollowAllowsCompassToPointNorth)
{
  auto const screen = MakeScreen();
  PositionListener listener;
  df::MyPositionController controller({location::FollowAndRotate, 0.0, {}, false, false, {}}, nullptr);
  controller.SetListener(make_ref(&listener));
  controller.OnUpdateScreen(screen);
  controller.OnLocationUpdate(MakeLocation(), false, screen);
  controller.SetLocationFollowPaused(true);
  listener.m_viewportChanges = 0;

  controller.OnCompassTapped();
  TEST_EQUAL(controller.GetCurrentMode(), location::Follow, ());
  TEST_EQUAL(listener.m_viewportChanges, 1, ());
  TEST_EQUAL(listener.m_rotations, 1, ());
  controller.UpdatePosition();
  TEST_EQUAL(listener.m_viewportChanges, 1, ());
  controller.SetLocationFollowPaused(false);
  TEST_EQUAL(listener.m_viewportChanges, 2, ());
}

UNIT_CLASS_TEST(VisualParamsFixture, StopFollowCancelsPauseWithoutRestoringFollow)
{
  auto const screen = MakeScreen();
  PositionListener listener;
  df::MyPositionController controller({location::Follow, 0.0, {}, false, false, {}}, nullptr);
  controller.SetListener(make_ref(&listener));
  controller.OnUpdateScreen(screen);
  controller.OnLocationUpdate(MakeLocation(), false, screen);
  controller.SetLocationFollowPaused(true);
  controller.StopLocationFollow();
  listener.m_viewportChanges = 0;
  controller.SetLocationFollowPaused(false);
  controller.UpdatePosition();
  TEST_EQUAL(controller.GetCurrentMode(), location::NotFollow, ());
  TEST_EQUAL(listener.m_viewportChanges, 0, ());
}

UNIT_CLASS_TEST(VisualParamsFixture, PauseFollowEndsOnRoutingTransitions)
{
  auto const screen = MakeScreen();
  PositionListener listener;
  df::MyPositionController controller({location::Follow, 0.0, {}, false, false, {}}, nullptr);
  controller.SetListener(make_ref(&listener));
  controller.OnUpdateScreen(screen);
  controller.OnLocationUpdate(MakeLocation(), false, screen);
  controller.SetLocationFollowPaused(true);
  listener.m_viewportChanges = 0;
  controller.ActivateRouting(16, false, false);
  TEST(controller.IsRouteFollowingActive(), ());
  TEST_EQUAL(listener.m_viewportChanges, 1, ());

  controller.SetLocationFollowPaused(true);
  listener.m_viewportChanges = 0;
  controller.DeactivateRouting();
  TEST_EQUAL(controller.GetCurrentMode(), location::Follow, ());
  TEST(controller.IsModeChangeViewport(), ());
  TEST_EQUAL(listener.m_viewportChanges, 1, ());
}
}  // namespace my_position_controller_tests
