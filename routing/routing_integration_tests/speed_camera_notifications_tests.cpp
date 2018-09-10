#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "routing/routing_tests/tools.hpp"

#include "routing/route.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/routing_session.hpp"

#include "platform/location.hpp"
#include "platform/measurement_utils.hpp"

#include "geometry/point2d.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using namespace routing;
using namespace routing::turns;
using namespace std;

namespace
{
string const kCameraOnTheWay = "Speed camera on the way";

location::GpsInfo MoveTo(ms::LatLon const & coords, double speed = -1)
{
  static auto constexpr kGpsAccuracy = 0.01;

  location::GpsInfo info;
  info.m_horizontalAccuracy = kGpsAccuracy;
  info.m_verticalAccuracy = kGpsAccuracy;
  info.m_latitude = coords.lat;
  info.m_longitude = coords.lon;
  info.m_speedMpS = speed;
  return info;
}

void ChangePosition(ms::LatLon const & coords, double speedKmPH, RoutingSession & routingSession)
{
  routingSession.OnLocationPositionChanged(MoveTo({coords.lat, coords.lon}, KMPH2MPS(speedKmPH)));
}

void InitRoutingSession(ms::LatLon const & from, ms::LatLon const & to, RoutingSession & routingSession)
{
  TRouteResult const routeResult =
    integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                MercatorBounds::FromLatLon(from), m2::PointD::Zero(),
                                MercatorBounds::FromLatLon(to));

  Route & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());

  routingSession.Init(nullptr /* RoutingStatisticsCallback */,
                      nullptr /* PointCheckCallback */);
  routingSession.SetRoutingSettings(routing::GetRoutingSettings(routing::VehicleType::Car));
  routingSession.AssignRouteForTesting(make_shared<Route>(route), result);
  routingSession.SetTurnNotificationsUnits(measurement_utils::Units::Metric);
  routingSession.ToggleSpeedCameras(true);
  string const engShortJson = R"(
    {
      "unknown_camera": ")" + kCameraOnTheWay + R"("
    }
  )";
  routingSession.SetLocaleWithJsonForTesting(engShortJson, "en");
}

bool IsExistsInNotification(vector<string> const & notifications, string const & value)
{
  return any_of(notifications.begin(), notifications.end(), [&value](auto const & item) {
    return item == value;
  });
}

// These both tests, check notification when drived placed in dangerous zone
// of speed camera.
UNIT_TEST(SpeedCameraNotification_1)
{
  RoutingSession routingSession;
  InitRoutingSession({55.67931, 37.53268} /* from */,
                     {55.68764, 37.54508} /* to   */,
                     routingSession);

  {
    ChangePosition({55.68126, 37.53551}, 100.0 /* speedKmPH */, routingSession);
    vector<string> notifications;
    routingSession.GenerateNotifications(notifications);

    TEST(IsExistsInNotification(notifications, kCameraOnTheWay), ());
  }
}

UNIT_TEST(SpeedCameraNotification_2)
{
  RoutingSession routingSession;
  InitRoutingSession({55.74070, 37.61681} /* from */,
                     {55.74885, 37.61036} /* to   */,
                     routingSession);

  {
    ChangePosition({55.74505, 37.61384}, 100.0 /* speedKmPH */, routingSession);
    vector<string> notifications;
    routingSession.GenerateNotifications(notifications);

    TEST(IsExistsInNotification(notifications, kCameraOnTheWay), ());
  }
}

// This test about notification before dangerous zone of speed camera.
// It is case in which driver should slow down the speed.
UNIT_TEST(SpeedCameraNotification_3)
{
  RoutingSession routingSession;
  InitRoutingSession({55.76801, 37.59363} /* from */,
                     {55.75947, 37.58484} /* to   */,
                     routingSession);

  {
    ChangePosition({55.76766, 37.59260}, 100.0 /* speedKmPH */, routingSession);
    vector<string> notifications;
    routingSession.GenerateNotifications(notifications);

    TEST(!IsExistsInNotification(notifications, kCameraOnTheWay), ());
  }

  {
    ChangePosition({55.76589, 37.58999}, 100.0 /* speedKmPH */, routingSession);
    vector<string> notifications;
    routingSession.GenerateNotifications(notifications);

    TEST(IsExistsInNotification(notifications, kCameraOnTheWay), ());
  }
}

// Test about camera which is not part of way in OSM.
// This link (camera and way) was found by geometry index.
UNIT_TEST(SpeedCameraNotification_4)
{
  RoutingSession routingSession;
  InitRoutingSession({55.65601, 37.53822} /* from */,
                     {55.65760, 37.52312} /* to   */,
                     routingSession);

  {
    ChangePosition({55.65647, 37.53643}, 100.0 /* speedKmPH */, routingSession);
    vector<string> notifications;
    routingSession.GenerateNotifications(notifications);

    TEST(!IsExistsInNotification(notifications, kCameraOnTheWay), ());
  }
  {
    ChangePosition({55.65671, 37.53236}, 100.0 /* speedKmPH */, routingSession);
    vector<string> notifications;
    routingSession.GenerateNotifications(notifications);

    TEST(IsExistsInNotification(notifications, kCameraOnTheWay), ());
  }
}
}  // namespace
