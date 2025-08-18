#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "routing/routing_tests/tools.hpp"

#include "routing/route.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/routing_session.hpp"
#include "routing/speed_camera_manager.hpp"

#include "platform/location.hpp"
#include "platform/measurement_utils.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace speed_camera_notifications_tests
{
using namespace routing;
using namespace routing::turns;
using namespace std;

string const kCameraOnTheWay = "Speed camera on the way";

location::GpsInfo MoveTo(ms::LatLon const & coords, double speed = -1)
{
  static auto constexpr kGpsAccuracy = 0.01;

  location::GpsInfo info;
  info.m_horizontalAccuracy = kGpsAccuracy;
  info.m_verticalAccuracy = kGpsAccuracy;
  info.m_latitude = coords.m_lat;
  info.m_longitude = coords.m_lon;
  info.m_speed = speed;
  return info;
}

void ChangePosition(ms::LatLon const & coords, double speedKmPH, RoutingSession & routingSession)
{
  routingSession.OnLocationPositionChanged(
      MoveTo({coords.m_lat, coords.m_lon}, measurement_utils::KmphToMps(speedKmPH)));
}

void InitRoutingSession(ms::LatLon const & from, ms::LatLon const & to, RoutingSession & routingSession,
                        SpeedCameraManagerMode mode = SpeedCameraManagerMode::Auto)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car), mercator::FromLatLon(from),
                                  m2::PointD::Zero(), mercator::FromLatLon(to));

  Route & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());

  routingSession.Init(nullptr /* PointCheckCallback */);
  routingSession.SetRoutingSettings(routing::GetRoutingSettings(routing::VehicleType::Car));
  routingSession.AssignRouteForTesting(make_shared<Route>(route), result);
  routingSession.SetTurnNotificationsUnits(measurement_utils::Units::Metric);
  routingSession.GetSpeedCamManager().SetMode(mode);
  string const engShortJson = R"(
    {
      "unknown_camera": ")" + kCameraOnTheWay +
                              R"("
    }
  )";
  routingSession.SetLocaleWithJsonForTesting(engShortJson, "en");
}

bool CheckVoiceNotification(RoutingSession & routingSession)
{
  vector<string> notifications;
  routingSession.GenerateNotifications(notifications, false);
  return any_of(notifications.begin(), notifications.end(), [](auto const & item) { return item == kCameraOnTheWay; });
}

bool CheckBeepSignal(RoutingSession & routingSession)
{
  return routingSession.GetSpeedCamManager().ShouldPlayBeepSignal();
}

SpeedCameraManager::Interval CheckZone(RoutingSession const & routingSession, double speedKmPH)
{
  SpeedCameraOnRoute const & closestCamera = routingSession.GetSpeedCamManager().GetClosestCamForTests();
  TEST(closestCamera.IsValid(), ("No speed camera found."));

  double const speedMpS = measurement_utils::KmphToMps(speedKmPH);
  double const passedDist = routingSession.GetRouteForTests()->GetCurrentDistanceFromBeginMeters();
  double const distToCamera = closestCamera.m_distFromBeginMeters - passedDist;

  return SpeedCameraManager::GetIntervalByDistToCam(distToCamera, speedMpS);
}

bool NoCameraFound(RoutingSession & routingSession)
{
  SpeedCameraOnRoute const & closestCamera = routingSession.GetSpeedCamManager().GetClosestCamForTests();
  return !closestCamera.IsValid();
}

// Mode: Auto/Always
// ____Notification___|___beep____|_____(exceed speed limit here) Impact camera zone_____|
// Expected: Beep signal.
/* Camera was (temporary) removed from OSM.
UNIT_TEST(SpeedCameraNotification_AutoAlwaysMode_1)
{
  vector<SpeedCameraManagerMode> modes = {SpeedCameraManagerMode::Auto, SpeedCameraManagerMode::Always};
  for (auto const mode : modes)
  {
    RoutingSession routingSession;
    InitRoutingSession({55.67931, 37.53268}, {55.68764, 37.54508},
                       routingSession, mode);

    {
      double const speedKmPH = 100.0;
      ChangePosition({55.68126, 37.53551}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::ImpactZone, ());
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(CheckBeepSignal(routingSession), ());
    }
  }
}
*/

// Mode: Auto/Always
// ____Notification___|___beep____|_____(exceed speed limit here) Impact camera zone_____|
// Expected: Beep signal.
UNIT_TEST(SpeedCameraNotification_AutoAlwaysMode_2)
{
  vector<SpeedCameraManagerMode> modes = {SpeedCameraManagerMode::Auto, SpeedCameraManagerMode::Always};
  for (auto const mode : modes)
  {
    RoutingSession routingSession;
    InitRoutingSession({55.74070, 37.61681} /* from */, {55.74885, 37.61036} /* to   */, routingSession, mode);

    {
      double const speedKmPH = 100.0;
      ChangePosition({55.74505, 37.61384}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::ImpactZone, ());
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(CheckBeepSignal(routingSession), ());
    }
  }
}

// Mode: Auto/Always
// ____Notification___|___(exceed speed limit here) beep____|_____Impact camera zone_____|
// Expected: Beep signal.
UNIT_TEST(SpeedCameraNotification_AutoAlwaysMode_3)
{
  vector<SpeedCameraManagerMode> modes = {SpeedCameraManagerMode::Auto, SpeedCameraManagerMode::Always};
  for (auto const mode : modes)
  {
    RoutingSession routingSession;
    InitRoutingSession({55.76801, 37.59363} /* from */, {55.75947, 37.58484} /* to   */, routingSession, mode);

    // No danger here.
    {
      double const speedKmPH = 100.0;
      ChangePosition({55.76766, 37.59260}, speedKmPH, routingSession);
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }

    // Exceed speed limit in beep zone.
    {
      double const speedKmPH = 100.0;
      ChangePosition({55.76589, 37.58999}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::BeepSignalZone, ());
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(CheckBeepSignal(routingSession), ());
    }
  }
}

// Next tests about camera which is not part of way in OSM.
// This link (camera and way) was found by geometry index.

// Mode: Auto/Always
// ____Notification___|___beep____|_____(exceed speed limit here) Impact camera zone_____|
// Expected: Beep signal.
UNIT_TEST(SpeedCameraNotification_AutoAlwaysMode_4)
{
  vector<SpeedCameraManagerMode> modes = {SpeedCameraManagerMode::Auto, SpeedCameraManagerMode::Always};
  for (auto const mode : modes)
  {
    RoutingSession routingSession;
    InitRoutingSession({55.65601, 37.53822} /* from */, {55.65760, 37.52312} /* to   */, routingSession, mode);

    {
      double const speedKmPH = 100.0;
      ChangePosition({55.65647, 37.53643}, speedKmPH, routingSession);
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }
    {
      double const speedKmPH = 100.0;
      ChangePosition({55.65671, 37.53236}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::ImpactZone, ());
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(CheckBeepSignal(routingSession), ());
    }
  }
}

// Mode: Auto/Always
// ____(exceed speed limit here) Notification___|___beep____|_____Impact camera zone_____|
// Expected: Voice notification.
UNIT_TEST(SpeedCameraNotification_AutoAlwaysMode_5)
{
  vector<SpeedCameraManagerMode> modes = {SpeedCameraManagerMode::Auto, SpeedCameraManagerMode::Always};
  for (auto const mode : modes)
  {
    RoutingSession routingSession;
    InitRoutingSession({55.76801, 37.59363} /* from */, {55.75947, 37.58484} /* to   */, routingSession, mode);

    // No danger here.
    {
      double const speedKmPH = 100.0;
      ChangePosition({55.76766, 37.59260}, speedKmPH, routingSession);
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }

    // Exceed speed limit before beep zone.
    {
      double const speedKmPH = 100.0;
      ChangePosition({55.76605, 37.59078}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::VoiceNotificationZone, ());
      TEST(CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }
  }
}

// Mode: Auto/Always
// ____(exceed speed limit here) Notification___|___(and here) beep____|_____Impact camera zone_____|
// Expected: Voice notification, after it beep signal.
UNIT_TEST(SpeedCameraNotification_AutoAlwaysMode_6)
{
  vector<SpeedCameraManagerMode> modes = {SpeedCameraManagerMode::Auto, SpeedCameraManagerMode::Always};
  for (auto const mode : modes)
  {
    RoutingSession routingSession;
    InitRoutingSession({55.76801, 37.59363} /* from */, {55.75947, 37.58484} /* to   */, routingSession, mode);

    // Exceed speed limit before beep zone.
    {
      double const speedKmPH = 100.0;
      ChangePosition({55.76605, 37.59078}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::VoiceNotificationZone, ());
      TEST(CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }

    // Exceed speed limit in beep zone.
    {
      double const speedKmPH = 100.0;
      ChangePosition({55.76560, 37.59015}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::BeepSignalZone, ());
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(CheckBeepSignal(routingSession), ());
    }
  }
}

// Mode: Auto/Always
// ___(exceed speed limit here) Notification___|___(not exceed speed limit here) beep____|____Impact camera zone____|
// We must hear beep signal after voice notification, no matter whether we exceed speed limit or not.
// Expected: Voice notification, after it beep signal.
UNIT_TEST(SpeedCameraNotification_AutoAlwaysMode_7)
{
  vector<SpeedCameraManagerMode> modes = {SpeedCameraManagerMode::Auto, SpeedCameraManagerMode::Always};
  for (auto const mode : modes)
  {
    RoutingSession routingSession;
    InitRoutingSession({55.76801, 37.59363} /* from */, {55.75947, 37.58484} /* to   */, routingSession, mode);

    // Exceed speed limit before beep zone.
    {
      double const speedKmPH = 100.0;
      ChangePosition({55.76655, 37.59146}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::VoiceNotificationZone, ());
      TEST(CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }

    // Intermediate Move for correct calculating of passedDistance.
    {
      double const speedKmPH = 40.0;
      ChangePosition({55.76602, 37.59074}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::VoiceNotificationZone, ());
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }

    // Intermediate Move for correct calculating of passedDistance.
    {
      double const speedKmPH = 20.0;
      ChangePosition({55.76559, 37.59016}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::VoiceNotificationZone, ());
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }

    // No exceed speed limit in beep zone, but we did VoiceNotification earlier,
    // so now we make BeedSignal.
    {
      double const speedKmPH = 40.0;
      ChangePosition({55.76573, 37.59030}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::BeepSignalZone, ());
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(CheckBeepSignal(routingSession), ());
    }
  }
}

// Mode: Always/Auto
// ____Notification___|___beep____|_____Impact camera zone_____|
// ----------------^  |  - We are here. Exceed speed limit.
//                    |    In case Always/Auto mode we should hear voice notification.
// -----------------^ |  - Then we are here. Exceed speed limit.
//                    |    But it's soon to make beep signal.
// ---------------------^ - Than we are here. Exceed speed limit.
//                          We should here beep signal.
UNIT_TEST(SpeedCameraNotification_AutoAlwaysMode_8)
{
  for (auto const mode : {SpeedCameraManagerMode::Auto, SpeedCameraManagerMode::Always})
  {
    // On "Leningradskiy" from East to West direction.
    RoutingSession routingSession;
    InitRoutingSession({55.6755737, 37.5264126},  // from
                       {55.67052, 37.51893},      // to
                       routingSession, mode);

    {
      double const speedKmPH = 180.0;
      ChangePosition({55.67533, 37.5264}, speedKmPH, routingSession);
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }
    {
      double const speedKmPH = 180.0;
      ChangePosition({55.67515, 37.52577}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::VoiceNotificationZone, ());
      TEST(CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }
    {
      double const speedKmPH = 180.0;
      ChangePosition({55.67515, 37.52577}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::VoiceNotificationZone, ());
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }
    {
      double const speedKmPH = 180.0;
      ChangePosition({55.67505, 37.52542}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::BeepSignalZone, ());
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(CheckBeepSignal(routingSession), ());
    }
  }
}

// Mode: Always
// ____Notification___|___beep____|_____Impact camera zone_____|
// --------------------------------^ - We are here. No exceed speed limit.
//                                     In case |Always| mode we should hear voice notification.
// Expected: Voice notification in |Always| mode.
UNIT_TEST(SpeedCameraNotification_AlwaysMode_1)
{
  {
    RoutingSession routingSession;
    InitRoutingSession({55.76801, 37.59363} /* from */, {55.75947, 37.58484} /* to   */, routingSession,
                       SpeedCameraManagerMode::Always);

    {
      double const speedKmPH = 40.0;
      ChangePosition({55.76476, 37.58905}, speedKmPH, routingSession);
      TEST_EQUAL(CheckZone(routingSession, speedKmPH), SpeedCameraManager::Interval::ImpactZone, ());
      TEST(CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }
  }
}

// Mode: Auto
// ____Notification___|___beep____|_____Impact camera zone_____|
// --------------------------------^ - We are here. No exceed speed limit.
//                                     In case |Auto| mode we should hear nothing.
// Expected: and nothing in mode: |Auto|.
UNIT_TEST(SpeedCameraNotification_AutoMode_1)
{
  {
    RoutingSession routingSession;
    InitRoutingSession({55.76801, 37.59363} /* from */, {55.75947, 37.58484} /* to   */, routingSession,
                       SpeedCameraManagerMode::Auto);

    {
      double const speedKmPH = 40.0;
      ChangePosition({55.76476, 37.58905}, speedKmPH, routingSession);
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }
  }
}

UNIT_TEST(SpeedCameraNotification_NeverMode_1)
{
  {
    RoutingSession routingSession;
    InitRoutingSession({55.76801, 37.59363} /* from */, {55.75947, 37.58484} /* to   */, routingSession,
                       SpeedCameraManagerMode::Never);

    {
      double const speedKmPH = 100.0;
      ChangePosition({55.76476, 37.58905}, speedKmPH, routingSession);
      TEST(!NoCameraFound(routingSession), ());
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }

    {
      double const speedKmPH = 200.0;
      ChangePosition({55.76441, 37.58865}, speedKmPH, routingSession);
      TEST(!NoCameraFound(routingSession), ());
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }

    {
      double const speedKmPH = 300.0;
      ChangePosition({55.76335, 37.58767}, speedKmPH, routingSession);
      TEST(!NoCameraFound(routingSession), ());
      TEST(!CheckVoiceNotification(routingSession), ());
      TEST(!CheckBeepSignal(routingSession), ());
    }
  }
}

// Test on case when a feature is split by a mini_roundabout or by a turning_loop and
// contains a speed camera. The thing is to pass this test it's necessary to process
// fake road feature ids correctly while speed cameras generation process.
UNIT_TEST(SpeedCameraNotification_CameraOnMiniRoundabout)
{
  RoutingSession routingSession;
  InitRoutingSession({41.201998, 69.109587} /* from */, {41.200358, 69.107051} /* to   */, routingSession,
                     SpeedCameraManagerMode::Never);
  double const speedKmPH = 100.0;
  ChangePosition({41.201998, 69.109587}, speedKmPH, routingSession);
  TEST(!NoCameraFound(routingSession), ());
}
}  // namespace speed_camera_notifications_tests
