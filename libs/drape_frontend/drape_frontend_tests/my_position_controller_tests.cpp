#include "testing/testing.hpp"

#include "drape_frontend/my_position_controller.hpp"
#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/visual_params.hpp"

#include "geometry/screenbase.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <utility>
#include <vector>

namespace my_position_controller_tests
{
double constexpr kAzimuthEps = 1e-9;

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

using ModeLog = std::vector<std::pair<location::EMyPositionMode, bool>>;
using OrientationLog = std::vector<location::MapOrientation>;

df::MyPositionController::Params MakeParams(location::EMyPositionMode initMode, bool isRoutingActive,
                                            location::MapOrientation routingOrientation, ModeLog * modeLog = nullptr,
                                            OrientationLog * orientationLog = nullptr)
{
  location::TMyPositionModeChanged fn;
  if (modeLog)
    fn = [modeLog](location::EMyPositionMode mode, bool routingActive) { modeLog->emplace_back(mode, routingActive); };
  location::TRoutingOrientationChanged orientationFn;
  if (orientationLog)
    orientationFn = [orientationLog](location::MapOrientation orientation) { orientationLog->push_back(orientation); };
  return {initMode,      routingOrientation,      0.0 /* timeInBackground */,
          df::Hints(),   isRoutingActive,         false /* isAutozoomEnabled */,
          std::move(fn), std::move(orientationFn)};
}

// Records every camera request so tests can assert not only the zoom but also which
// ChangeModelView() overload was used, the requested azimuth and the pixel anchor.
class TestListener : public df::MyPositionController::Listener
{
public:
  enum class Camera
  {
    Center,
    Azimuth,
    Rect,
    FollowAndRotate,
    AutoZoom
  };

  struct CameraRequest
  {
    Camera m_type;
    double m_azimuth = 0.0;
    m2::PointD m_pxZero = m2::PointD::Zero();
    int m_zoomLevel = df::kDoNotChangeZoom;
  };

  void PositionChanged(m2::PointD const & /* position */, bool /* hasPosition */) override {}

  void ChangeModelView(m2::PointD const & /* center */, int zoomLevel,
                       df::TAnimationCreator const & /* parallelAnimCreator */) override
  {
    m_requests.push_back({Camera::Center, 0.0, m2::PointD::Zero(), zoomLevel});
  }

  void ChangeModelView(double azimuth, df::TAnimationCreator const & /* parallelAnimCreator */) override
  {
    m_requests.push_back({Camera::Azimuth, azimuth, m2::PointD::Zero(), df::kDoNotChangeZoom});
  }

  void ChangeModelView(m2::RectD const & /* rect */, df::TAnimationCreator const & /* parallelAnimCreator */) override
  {
    m_requests.push_back({Camera::Rect, 0.0, m2::PointD::Zero(), df::kDoNotChangeZoom});
  }

  void ChangeModelView(m2::PointD const & /* userPos */, double azimuth, m2::PointD const & pxZero, int zoomLevel,
                       df::Animation::TAction const & /* onFinishAction */,
                       df::TAnimationCreator const & /* parallelAnimCreator */) override
  {
    m_requests.push_back({Camera::FollowAndRotate, azimuth, pxZero, zoomLevel});
  }

  void ChangeModelView(double /* autoScale */, m2::PointD const & /* userPos */, double azimuth,
                       m2::PointD const & pxZero, df::TAnimationCreator const & /* parallelAnimCreator */) override
  {
    m_requests.push_back({Camera::AutoZoom, azimuth, pxZero, df::kDoNotChangeZoom});
  }

  CameraRequest const & LastRequest() const
  {
    TEST(!m_requests.empty(), ());
    return m_requests.back();
  }

  std::vector<CameraRequest> m_requests;
};

std::string DebugPrint(TestListener::Camera camera)
{
  switch (camera)
  {
  case TestListener::Camera::Center: return "Center";
  case TestListener::Camera::Azimuth: return "Azimuth";
  case TestListener::Camera::Rect: return "Rect";
  case TestListener::Camera::FollowAndRotate: return "FollowAndRotate";
  case TestListener::Camera::AutoZoom: return "AutoZoom";
  }
  UNREACHABLE();
}

class Controller
{
public:
  Controller(location::EMyPositionMode initMode, bool isRoutingActive,
             location::MapOrientation routingOrientation = location::MapOrientation::HeadingUp)
  {
    df::VisualParams::Init(1.0, 1024);
    m_screen = MakeScreen();
    m_controller = std::make_unique<df::MyPositionController>(
        MakeParams(initMode, isRoutingActive, routingOrientation, &m_modeLog, &m_orientationLog),
        ref_ptr<df::DrapeNotifier>());
    m_controller->SetListener(ref_ptr<df::MyPositionController::Listener>(&m_listener));
    m_controller->OnUpdateScreen(m_screen);
  }

  df::MyPositionController & Get() { return *m_controller; }
  void OnLocationUpdate(double timestamp) { m_controller->OnLocationUpdate(MakeGpsInfo(timestamp), true, m_screen); }
  void NextMode() { m_controller->NextMode(m_screen); }

  m2::PointD ScreenCenter() const { return m_screen.PixelRectIn3d().Center(); }
  m2::PointD RoutingCenter() const
  {
    double const offsetY = 104 * df::VisualParams::Instance().GetVisualScale();
    return {ScreenCenter().x, m_screen.PixelRectIn3d().maxY() - offsetY};
  }

  TestListener m_listener;
  ModeLog m_modeLog;
  OrientationLog m_orientationLog;

private:
  ScreenBase m_screen;
  std::unique_ptr<df::MyPositionController> m_controller;
};

UNIT_TEST(MyPositionController_FirstFixAppliesDesiredInitMode)
{
  Controller test(location::NotFollow, false /* isRoutingActive */);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::PendingPosition, ());
  TEST(!test.Get().IsModeHasPosition(), ());

  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::NotFollow, ());
  TEST(test.Get().IsModeHasPosition(), ());
  // NotFollow does not own the camera.
  TEST_EQUAL(test.m_listener.m_requests.size(), 0, ());

  ModeLog const expected = {{location::PendingPosition, false}, {location::NotFollow, false}};
  TEST_EQUAL(test.m_modeLog, expected, ());
}

UNIT_TEST(MyPositionController_InitNotFollowNoPositionStaysStopped)
{
  Controller test(location::NotFollowNoPosition, false /* isRoutingActive */);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::NotFollowNoPosition, ());

  // The location button restarts the search; the first fix then centers on the user.
  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::PendingPosition, ());

  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());
  TEST_EQUAL(test.m_listener.LastRequest().m_type, TestListener::Camera::Center, ());
  TEST_EQUAL(test.m_listener.LastRequest().m_zoomLevel, df::kDoNotChangeZoom, ());
}

UNIT_TEST(MyPositionController_FirstFixAppliesFollowAndRotate)
{
  Controller test(location::FollowAndRotate, false /* isRoutingActive */);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::PendingPosition, ());

  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());

  auto const & request = test.m_listener.LastRequest();
  TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
  TEST_ALMOST_EQUAL_ABS(request.m_azimuth, math::DegToRad(45.0), kAzimuthEps, ());
  TEST_EQUAL(request.m_pxZero, test.ScreenCenter(), ());
}

UNIT_TEST(MyPositionController_ReacquisitionDropsRotation)
{
  Controller test(location::FollowAndRotate, false /* isRoutingActive */);
  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());

  test.Get().LoseLocation();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::PendingPosition, ());
  TEST(!test.Get().IsModeHasPosition(), ());

  // Reacquisition outside routing lands in plain Follow: the rotation preference is not restored.
  test.OnLocationUpdate(2.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());
  TEST_EQUAL(test.m_listener.LastRequest().m_type, TestListener::Camera::Center, ());
}

UNIT_TEST(MyPositionController_NextModeCycle)
{
  Controller test(location::Follow, false /* isRoutingActive */);
  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());

  // The GPS bearing made rotation available.
  TEST(test.Get().IsRotationAvailable(), ());
  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_ALMOST_EQUAL_ABS(request.m_azimuth, math::DegToRad(45.0), kAzimuthEps, ());
    TEST_EQUAL(request.m_pxZero, test.ScreenCenter(), ());
  }

  // Switching back to Follow explicitly resets the map to north-up.
  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_EQUAL(request.m_azimuth, 0.0, ());
    TEST_EQUAL(request.m_pxZero, test.ScreenCenter(), ());
  }
}

UNIT_TEST(MyPositionController_NextModeWhilePendingStopsAndRestarts)
{
  Controller test(location::NotFollow, false /* isRoutingActive */);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::PendingPosition, ());

  // Pressing the location button while waiting for a fix stops the search.
  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::NotFollowNoPosition, ());

  // The next press restarts it and re-arms Follow instead of the saved NotFollow.
  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::PendingPosition, ());

  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());
}

UNIT_TEST(MyPositionController_RoutingPendingButtonsKeepOrientation)
{
  Controller test(location::FollowAndRotate, true /* isRoutingActive */, location::MapOrientation::HeadingUp);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::PendingPosition, ());

  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::NotFollowNoPosition, ());
  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::PendingPosition, ());

  // Stopping and restarting the location search before the first fix must not lose
  // the heading-up navigation camera, and restoring it is not an orientation choice.
  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());
  TEST(test.m_orientationLog.empty(), ());
}

UNIT_TEST(MyPositionController_RoutingQueuedFixAfterStopKeepsOrientation)
{
  Controller test(location::FollowAndRotate, true /* isRoutingActive */, location::MapOrientation::HeadingUp);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::PendingPosition, ());

  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::NotFollowNoPosition, ());

  // A location that was already queued arrives right after the "stop location" press.
  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());
  TEST(test.m_orientationLog.empty(), ());
}

UNIT_TEST(MyPositionController_RoutingRestoreFromLocationOffResumesFollowing)
{
  // Restoring into active navigation (isRoutingActive means the route is being followed) with a
  // saved "location off" mode must resume the preferred following camera and re-arm the location
  // search: navigation is meaningless without a position, so the map cannot stay stopped.
  Controller test(location::NotFollowNoPosition, true /* isRoutingActive */, location::MapOrientation::NorthUp);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::PendingPosition, ());

  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());
  TEST(test.Get().IsModeHasPosition(), ());
  TEST(test.Get().IsRouteFollowingActive(), ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_EQUAL(request.m_azimuth, 0.0, ());
    TEST_EQUAL(request.m_pxZero, test.ScreenCenter(), ());
  }
  // Restoring the saved orientation is not an orientation choice.
  TEST(test.m_orientationLog.empty(), ());
}

UNIT_TEST(MyPositionController_RoutingRecoveryRestoresHeadingUpByDefault)
{
  Controller test(location::FollowAndRotate, true /* isRoutingActive */);
  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_EQUAL(request.m_pxZero, test.RoutingCenter(), ());
  }

  test.Get().LoseLocation();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::PendingPosition, ());

  // Routing recovery restores the preferred orientation — heading-up by default here — with the routing zoom.
  test.OnLocationUpdate(2.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_ALMOST_EQUAL_ABS(request.m_azimuth, math::DegToRad(45.0), kAzimuthEps, ());
    TEST_EQUAL(request.m_pxZero, test.RoutingCenter(), ());
    TEST_EQUAL(request.m_zoomLevel, 16, ());
  }
}

UNIT_TEST(MyPositionController_ActivateRoutingDefaultsToHeadingUp)
{
  Controller test(location::NotFollow, false /* isRoutingActive */);
  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::NotFollow, ());

  test.Get().ActivateRouting(16 /* zoomLevel */, 17 /* zoomLevelIn3d */, false /* enableAutoZoom */,
                             true /* isArrowGlued */);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());
  TEST(test.Get().IsRouteFollowingActive(), ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_ALMOST_EQUAL_ABS(request.m_azimuth, math::DegToRad(45.0), kAzimuthEps, ());
    TEST_EQUAL(request.m_pxZero, test.RoutingCenter(), ());
    TEST_EQUAL(request.m_zoomLevel, 16, ());
  }

  // Panning away pauses following; the location button restores the preferred heading-up orientation.
  test.Get().StopLocationFollow();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::NotFollow, ());
  TEST(!test.Get().IsRouteFollowingActive(), ());

  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());
  TEST_EQUAL(test.m_listener.LastRequest().m_pxZero, test.RoutingCenter(), ());

  // Finishing the route resets the camera to north-up Follow.
  test.Get().DeactivateRouting();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_EQUAL(request.m_azimuth, 0.0, ());
    TEST_EQUAL(request.m_pxZero, test.ScreenCenter(), ());
  }
}

UNIT_TEST(MyPositionController_ActivateRoutingBeforeFirstFix)
{
  Controller test(location::NotFollow, false /* isRoutingActive */);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::PendingPosition, ());

  // Routing activation claims the position even though there was no fix yet.
  test.Get().ActivateRouting(16 /* zoomLevel */, 17 /* zoomLevelIn3d */, false /* enableAutoZoom */,
                             true /* isArrowGlued */);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());
  TEST(test.Get().IsModeHasPosition(), ());

  // The first fix keeps the routing camera instead of the stale pre-routing desired mode.
  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_EQUAL(request.m_pxZero, test.RoutingCenter(), ());
  }
}

UNIT_TEST(MyPositionController_LoseLocationWhileNotFollow)
{
  Controller test(location::NotFollow, false /* isRoutingActive */);
  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::NotFollow, ());

  test.Get().LoseLocation();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::NotFollowNoPosition, ());

  // A stray fix while stopped is accepted silently.
  test.OnLocationUpdate(2.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::NotFollow, ());
  TEST_EQUAL(test.m_listener.m_requests.size(), 0, ());
}

UNIT_TEST(MyPositionController_CompassTappedResetsNorth)
{
  Controller test(location::FollowAndRotate, false /* isRoutingActive */);
  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());

  test.Get().OnCompassTapped();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_EQUAL(request.m_azimuth, 0.0, ());
    TEST_EQUAL(request.m_pxZero, test.ScreenCenter(), ());
    TEST_EQUAL(request.m_zoomLevel, df::kDoNotChangeZoom, ());
  }

  // In Follow the compass tap only rotates the map back to north.
  test.Get().OnCompassTapped();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::Azimuth, ());
    TEST_EQUAL(request.m_azimuth, 0.0, ());
  }
}

UNIT_TEST(MyPositionController_ModeCallbackSequence)
{
  // The saved north-up routing orientation overrides the saved mode while routing is active.
  Controller test(location::FollowAndRotate, true /* isRoutingActive */, location::MapOrientation::NorthUp);
  test.OnLocationUpdate(1.0);
  test.Get().StopLocationFollow();
  test.NextMode();
  test.Get().DeactivateRouting();

  ModeLog const expected = {{location::PendingPosition, true},
                            {location::Follow, true},
                            {location::NotFollow, true},
                            {location::Follow, true},
                            {location::Follow, false}};
  TEST_EQUAL(test.m_modeLog, expected, ());
  TEST(test.m_orientationLog.empty(), ());
}

UNIT_TEST(MyPositionController_RoutingStartKeepsSavedNorthUp)
{
  Controller test(location::NotFollow, false /* isRoutingActive */, location::MapOrientation::NorthUp);
  test.Get().EnablePerspectiveInRouting(true /* enablePerspective */);
  test.OnLocationUpdate(1.0);

  test.Get().ActivateRouting(16 /* zoomLevel */, 17 /* zoomLevelIn3d */, false /* enableAutoZoom */,
                             true /* isArrowGlued */);

  // North-up navigation starts in Follow with the 2D zoom, an explicit north azimuth
  // and the user centered on the screen.
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());
  TEST(test.Get().IsRouteFollowingActive(), ());
  TEST(!test.Get().ShouldEnablePerspectiveInRouting(), ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_EQUAL(request.m_azimuth, 0.0, ());
    TEST_EQUAL(request.m_pxZero, test.ScreenCenter(), ());
    TEST_EQUAL(request.m_zoomLevel, 16, ());
  }
  // Restoring the saved orientation is not an orientation choice.
  TEST(test.m_orientationLog.empty(), ());
}

UNIT_TEST(MyPositionController_RoutingStartUses3dZoomForHeadingUp)
{
  Controller test(location::NotFollow, false /* isRoutingActive */, location::MapOrientation::HeadingUp);
  test.Get().EnablePerspectiveInRouting(true /* enablePerspective */);
  test.OnLocationUpdate(1.0);

  test.Get().ActivateRouting(16 /* zoomLevel */, 17 /* zoomLevelIn3d */, false /* enableAutoZoom */,
                             true /* isArrowGlued */);

  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());
  TEST(test.Get().ShouldEnablePerspectiveInRouting(), ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_ALMOST_EQUAL_ABS(request.m_azimuth, math::DegToRad(45.0), kAzimuthEps, ());
    TEST_EQUAL(request.m_pxZero, test.RoutingCenter(), ());
    TEST_EQUAL(request.m_zoomLevel, 17, ());
  }
}

UNIT_TEST(MyPositionController_RoutingOrientationSwitchIsExplicitAndPersisted)
{
  Controller test(location::FollowAndRotate, true /* isRoutingActive */, location::MapOrientation::HeadingUp);
  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());

  // The location button chooses north-up: the camera resets to north and the choice is reported.
  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_EQUAL(request.m_azimuth, 0.0, ());
    TEST_EQUAL(request.m_pxZero, test.ScreenCenter(), ());
  }
  OrientationLog expected = {location::MapOrientation::NorthUp};
  TEST_EQUAL(test.m_orientationLog, expected, ());

  // The next press chooses heading-up again.
  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());
  expected.push_back(location::MapOrientation::HeadingUp);
  TEST_EQUAL(test.m_orientationLog, expected, ());
}

UNIT_TEST(MyPositionController_RoutingCompassTapChoosesNorthUp)
{
  Controller test(location::FollowAndRotate, true /* isRoutingActive */, location::MapOrientation::HeadingUp);
  test.OnLocationUpdate(1.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());

  test.Get().OnCompassTapped();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_EQUAL(request.m_azimuth, 0.0, ());
  }
  OrientationLog const expected = {location::MapOrientation::NorthUp};
  TEST_EQUAL(test.m_orientationLog, expected, ());
}

UNIT_TEST(MyPositionController_RoutingNorthUpSurvivesLocationLoss)
{
  Controller test(location::FollowAndRotate, true /* isRoutingActive */, location::MapOrientation::HeadingUp);
  test.OnLocationUpdate(1.0);
  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());

  test.Get().LoseLocation();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::PendingPosition, ());

  // The recovery restores north-up with an explicit zero azimuth and the user centered.
  test.OnLocationUpdate(2.0);
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_EQUAL(request.m_azimuth, 0.0, ());
    TEST_EQUAL(request.m_pxZero, test.ScreenCenter(), ());
    TEST_EQUAL(request.m_zoomLevel, 16, ());
  }
  OrientationLog const expected = {location::MapOrientation::NorthUp};
  TEST_EQUAL(test.m_orientationLog, expected, ());
}

UNIT_TEST(MyPositionController_RoutingNotFollowReturnsToPreferredNorthUp)
{
  Controller test(location::FollowAndRotate, true /* isRoutingActive */, location::MapOrientation::HeadingUp);
  test.OnLocationUpdate(1.0);
  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());

  test.Get().StopLocationFollow();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::NotFollow, ());

  // Recentering restores the preferred north-up orientation with a zero azimuth.
  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());
  {
    auto const & request = test.m_listener.LastRequest();
    TEST_EQUAL(request.m_type, TestListener::Camera::FollowAndRotate, ());
    TEST_EQUAL(request.m_azimuth, 0.0, ());
    TEST_EQUAL(request.m_pxZero, test.ScreenCenter(), ());
  }
  // Only the explicit switch to north-up was reported, not the restorations.
  OrientationLog const expected = {location::MapOrientation::NorthUp};
  TEST_EQUAL(test.m_orientationLog, expected, ());
}

UNIT_TEST(MyPositionController_RoutingPerspectiveIsStableThroughTransientStates)
{
  Controller test(location::NotFollow, false /* isRoutingActive */, location::MapOrientation::HeadingUp);
  test.Get().EnablePerspectiveInRouting(true /* enablePerspective */);
  test.OnLocationUpdate(1.0);
  TEST(!test.Get().ShouldEnablePerspectiveInRouting(), ());

  test.Get().ActivateRouting(16 /* zoomLevel */, 17 /* zoomLevelIn3d */, false /* enableAutoZoom */,
                             true /* isArrowGlued */);
  TEST(test.Get().ShouldEnablePerspectiveInRouting(), ());

  // Neither a location loss, nor a manual pan, nor the recovery flattens the heading-up camera.
  test.Get().LoseLocation();
  TEST(test.Get().ShouldEnablePerspectiveInRouting(), ());
  test.OnLocationUpdate(2.0);
  TEST(test.Get().ShouldEnablePerspectiveInRouting(), ());
  test.Get().StopLocationFollow();
  TEST(test.Get().ShouldEnablePerspectiveInRouting(), ());
  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());
  TEST(test.Get().ShouldEnablePerspectiveInRouting(), ());

  // The 3D setting switch applies immediately.
  test.Get().EnablePerspectiveInRouting(false /* enablePerspective */);
  TEST(!test.Get().ShouldEnablePerspectiveInRouting(), ());
}

UNIT_TEST(MyPositionController_RoutingPerspectiveIsNeverEnabledForNorthUp)
{
  Controller test(location::NotFollow, false /* isRoutingActive */, location::MapOrientation::NorthUp);
  test.Get().EnablePerspectiveInRouting(true /* enablePerspective */);
  test.OnLocationUpdate(1.0);

  test.Get().ActivateRouting(16 /* zoomLevel */, 17 /* zoomLevelIn3d */, false /* enableAutoZoom */,
                             true /* isArrowGlued */);
  TEST(!test.Get().ShouldEnablePerspectiveInRouting(), ());

  test.Get().LoseLocation();
  TEST(!test.Get().ShouldEnablePerspectiveInRouting(), ());
  test.OnLocationUpdate(2.0);
  TEST(!test.Get().ShouldEnablePerspectiveInRouting(), ());
  test.Get().StopLocationFollow();
  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::Follow, ());
  TEST(!test.Get().ShouldEnablePerspectiveInRouting(), ());

  // Switching to heading-up enables the perspective again.
  test.NextMode();
  TEST_EQUAL(test.Get().GetCurrentMode(), location::FollowAndRotate, ());
  TEST(test.Get().ShouldEnablePerspectiveInRouting(), ());
}
}  // namespace my_position_controller_tests
