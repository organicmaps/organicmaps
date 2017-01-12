#import "MWMLocationManager.h"

#include "platform/location.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include "geometry/mercator.hpp"

namespace location_helpers
{
static inline char const * getSpeedSymbol(CLLocationSpeed const & metersPerSecond)
{
  // 0-1 m/s
  static char const * turtle = "\xF0\x9F\x90\xA2 ";
  // 1-2 m/s
  static char const * pedestrian = "\xF0\x9F\x9A\xB6 ";
  // 2-5 m/s
  static char const * tractor = "\xF0\x9F\x9A\x9C ";
  // 5-10 m/s
  static char const * bicycle = "\xF0\x9F\x9A\xB2 ";
  // 10-36 m/s
  static char const * car = "\xF0\x9F\x9A\x97 ";
  // 36-120 m/s
  static char const * train = "\xF0\x9F\x9A\x85 ";
  // 120-278 m/s
  static char const * airplane = "\xE2\x9C\x88\xEF\xB8\x8F ";
  // 278+
  static char const * rocket = "\xF0\x9F\x9A\x80 ";

  if (metersPerSecond <= 1.)
    return turtle;
  else if (metersPerSecond <= 2.)
    return pedestrian;
  else if (metersPerSecond <= 5.)
    return tractor;
  else if (metersPerSecond <= 10.)
    return bicycle;
  else if (metersPerSecond <= 36.)
    return car;
  else if (metersPerSecond <= 120.)
    return train;
  else if (metersPerSecond <= 278.)
    return airplane;
  else
    return rocket;
}

static inline NSString * formattedSpeedAndAltitude(CLLocation * location)
{
  if (!location)
    return nil;
  string result;
  if (location.altitude)
    result = "\xE2\x96\xB2 " /* this is simple mountain symbol */ +
             measurement_utils::FormatAltitude(location.altitude);
  // Speed is actual only for just received location
  if (location.speed > 0. && [location.timestamp timeIntervalSinceNow] >= -2.0)
  {
    if (!result.empty())
      result += "   ";
    result += getSpeedSymbol(location.speed) +
              measurement_utils::FormatSpeedWithDeviceUnits(location.speed);
  }
  return result.empty() ? nil : @(result.c_str());
}

static inline NSString * formattedDistance(double const & meters)
{
  if (meters < 0.)
    return nil;

  string s;
  measurement_utils::FormatDistance(meters, s);
  return @(s.c_str());
}

static inline BOOL isMyPositionPendingOrNoPosition()
{
  location::EMyPositionMode mode;
  if (!settings::Get(settings::kLocationStateMode, mode))
    return true;
  return mode == location::EMyPositionMode::PendingPosition ||
         mode == location::EMyPositionMode::NotFollowNoPosition;
}

static inline double headingToNorthRad(CLHeading * heading)
{
  double north = -1.0;
  if (heading)
  {
    north = (heading.trueHeading < 0) ? heading.magneticHeading : heading.trueHeading;
    north = my::DegToRad(north);
  }
  return north;
}

static inline ms::LatLon ToLatLon(m2::PointD const & p) { return MercatorBounds::ToLatLon(p); }

static inline m2::PointD ToMercator(CLLocationCoordinate2D const & l)
{
  return MercatorBounds::FromLatLon(l.latitude, l.longitude);
}

static inline m2::PointD ToMercator(ms::LatLon const & l) { return MercatorBounds::FromLatLon(l); }
static inline void setMyPositionMode(location::EMyPositionMode mode)
{
  MWMMyPositionMode mwmMode;
  switch (mode)
  {
  case location::EMyPositionMode::PendingPosition:
    mwmMode = MWMMyPositionModePendingPosition;
    break;
  case location::EMyPositionMode::NotFollowNoPosition:
    mwmMode = MWMMyPositionModeNotFollowNoPosition;
    break;
  case location::EMyPositionMode::NotFollow: mwmMode = MWMMyPositionModeNotFollow; break;
  case location::EMyPositionMode::Follow: mwmMode = MWMMyPositionModeFollow; break;
  case location::EMyPositionMode::FollowAndRotate:
    mwmMode = MWMMyPositionModeFollowAndRotate;
    break;
  }
  [MWMLocationManager setMyPositionMode:mwmMode];
}

}  // namespace MWMLocationHelpers
