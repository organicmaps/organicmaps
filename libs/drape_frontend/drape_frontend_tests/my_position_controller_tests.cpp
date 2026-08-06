#include "testing/testing.hpp"

#include "drape_frontend/my_position_controller.hpp"
#include "drape_frontend/visual_params.hpp"

#include "geometry/screenbase.hpp"

namespace my_position_controller_tests
{
ScreenBase MakeScreen()
{
  ScreenBase screen;
  screen.SetFromRects(m2::AnyRectD(m2::RectD(0.0, 0.0, 100.0, 100.0)), m2::RectD(0.0, 0.0, 1000.0, 1000.0));
  return screen;
}

location::GpsInfo MakeGpsInfo(double timestamp)
{
  location::GpsInfo info;
  info.m_source = location::EUser;
  info.m_timestamp = timestamp;
  info.m_latitude = 10.0;
  info.m_longitude = 20.0;
  info.m_horizontalAccuracy = 5.0;
  info.m_bearing = 45.0;
  info.m_speed = 5.0;
  return info;
}

df::MyPositionController::Params MakeParams(location::EMyPositionMode initMode, location::EMyPositionMode routingMode,
                                            bool isRoutingActive)
{
  return {initMode,
          routingMode,
          0.0 /* timeInBackground */,
          df::Hints(),
          isRoutingActive,
          false /* isAutozoomEnabled */,
          location::TMyPositionModeChanged()};
}

class TestListener : public df::MyPositionController::Listener
{
public:
  void PositionChanged(m2::PointD const & /* position */, bool /* hasPosition */) override {}

  void ChangeModelView(m2::PointD const & /* center */, int zoomLevel,
                       df::TAnimationCreator const & /* parallelAnimCreator */) override
  {
    m_lastZoomLevel = zoomLevel;
  }

  void ChangeModelView(double /* azimuth */, df::TAnimationCreator const & /* parallelAnimCreator */) override {}

  void ChangeModelView(m2::RectD const & /* rect */, df::TAnimationCreator const & /* parallelAnimCreator */) override
  {}

  void ChangeModelView(m2::PointD const & /* userPos */, double /* azimuth */, m2::PointD const & /* pxZero */,
                       int zoomLevel, df::Animation::TAction const & /* onFinishAction */,
                       df::TAnimationCreator const & /* parallelAnimCreator */) override
  {
    m_lastZoomLevel = zoomLevel;
  }

  void ChangeModelView(double /* autoScale */, m2::PointD const & /* userPos */, double /* azimuth */,
                       m2::PointD const & /* pxZero */,
                       df::TAnimationCreator const & /* parallelAnimCreator */) override
  {}

  int m_lastZoomLevel = -1;
};

UNIT_TEST(MyPositionController_RoutingStartKeepsSavedNorthUp)
{
  df::VisualParams::Init(1.0, 1024);

  auto params = MakeParams(location::NotFollow, location::Follow, false /* isRoutingActive */);
  df::MyPositionController controller(std::move(params), ref_ptr<df::DrapeNotifier>());
  controller.EnablePerspectiveInRouting(true /* enablePerspective */);

  TestListener listener;
  controller.SetListener(ref_ptr<df::MyPositionController::Listener>(&listener));

  controller.ActivateRouting(16 /* zoomLevel */, 17 /* zoomLevelIn3d */, false /* enableAutoZoom */,
                             true /* isArrowGlued */);

  TEST_EQUAL(controller.GetCurrentMode(), location::Follow, ());
  TEST(!controller.ShouldEnablePerspectiveInRouting(), ());
  TEST_EQUAL(listener.m_lastZoomLevel, 16, ());
}

UNIT_TEST(MyPositionController_RoutingStartUses3dZoomForHeadingUp)
{
  df::VisualParams::Init(1.0, 1024);

  auto params = MakeParams(location::NotFollow, location::FollowAndRotate, false /* isRoutingActive */);
  df::MyPositionController controller(std::move(params), ref_ptr<df::DrapeNotifier>());
  controller.EnablePerspectiveInRouting(true /* enablePerspective */);

  TestListener listener;
  controller.SetListener(ref_ptr<df::MyPositionController::Listener>(&listener));

  controller.ActivateRouting(16 /* zoomLevel */, 17 /* zoomLevelIn3d */, false /* enableAutoZoom */,
                             true /* isArrowGlued */);

  TEST_EQUAL(controller.GetCurrentMode(), location::FollowAndRotate, ());
  TEST(controller.ShouldEnablePerspectiveInRouting(), ());
  TEST_EQUAL(listener.m_lastZoomLevel, 17, ());
}

UNIT_TEST(MyPositionController_RoutingPerspectiveFollowsOrientation)
{
  df::VisualParams::Init(1.0, 1024);

  auto screen = MakeScreen();
  auto params = MakeParams(location::FollowAndRotate, location::FollowAndRotate, true /* isRoutingActive */);
  df::MyPositionController controller(std::move(params), ref_ptr<df::DrapeNotifier>());
  controller.EnablePerspectiveInRouting(true /* enablePerspective */);
  controller.OnUpdateScreen(screen);

  TEST(controller.ShouldEnablePerspectiveInRouting(), ());

  controller.OnLocationUpdate(MakeGpsInfo(1.0), true /* isNavigable */, screen);
  TEST_EQUAL(controller.GetCurrentMode(), location::FollowAndRotate, ());
  TEST(controller.ShouldEnablePerspectiveInRouting(), ());

  controller.NextMode(screen);
  TEST_EQUAL(controller.GetCurrentMode(), location::Follow, ());
  TEST(!controller.ShouldEnablePerspectiveInRouting(), ());

  controller.NextMode(screen);
  TEST_EQUAL(controller.GetCurrentMode(), location::FollowAndRotate, ());
  TEST(controller.ShouldEnablePerspectiveInRouting(), ());

  controller.EnablePerspectiveInRouting(false /* enablePerspective */);
  TEST(!controller.ShouldEnablePerspectiveInRouting(), ());
}

UNIT_TEST(MyPositionController_RoutingNorthUpSurvivesLocationLoss)
{
  df::VisualParams::Init(1.0, 1024);

  auto screen = MakeScreen();
  auto params = MakeParams(location::FollowAndRotate, location::FollowAndRotate, true /* isRoutingActive */);
  df::MyPositionController controller(std::move(params), ref_ptr<df::DrapeNotifier>());
  controller.OnUpdateScreen(screen);

  controller.OnLocationUpdate(MakeGpsInfo(1.0), true /* isNavigable */, screen);
  TEST_EQUAL(controller.GetCurrentMode(), location::FollowAndRotate, ());

  controller.NextMode(screen);
  TEST_EQUAL(controller.GetCurrentMode(), location::Follow, ());

  controller.LoseLocation();
  TEST_EQUAL(controller.GetCurrentMode(), location::PendingPosition, ());

  controller.OnLocationUpdate(MakeGpsInfo(2.0), true /* isNavigable */, screen);
  TEST_EQUAL(controller.GetCurrentMode(), location::Follow, ());
}

UNIT_TEST(MyPositionController_RoutingNotFollowReturnsToPreferredNorthUp)
{
  df::VisualParams::Init(1.0, 1024);

  auto screen = MakeScreen();
  auto params = MakeParams(location::FollowAndRotate, location::FollowAndRotate, true /* isRoutingActive */);
  df::MyPositionController controller(std::move(params), ref_ptr<df::DrapeNotifier>());
  controller.OnUpdateScreen(screen);
  controller.OnLocationUpdate(MakeGpsInfo(1.0), true /* isNavigable */, screen);

  controller.NextMode(screen);
  TEST_EQUAL(controller.GetCurrentMode(), location::Follow, ());

  controller.StopLocationFollow();
  TEST_EQUAL(controller.GetCurrentMode(), location::NotFollow, ());

  controller.NextMode(screen);
  TEST_EQUAL(controller.GetCurrentMode(), location::Follow, ());
}
}  // namespace my_position_controller_tests
