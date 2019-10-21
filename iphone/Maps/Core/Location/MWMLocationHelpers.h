#import "MWMLocationManager.h"
#import "SwiftBridge.h"

#include "platform/location.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include "geometry/mercator.hpp"

namespace location_helpers
{

static inline NSString * formattedSpeedAndAltitude(CLLocation * location)
{
  if (!location)
    return nil;
  NSMutableString * result = [@"" mutableCopy];
  if (location.altitude)
    [result appendString:[NSString stringWithFormat:@"%@ %@", @"\xE2\x96\xB2", @(measurement_utils::FormatAltitude(location.altitude).c_str())]];

  // Speed is actual only for just received location
  if (location.speed > 0. && location.timestamp.timeIntervalSinceNow >= -2.0)
  {
    if (result.length)
      [result appendString:@"  "];

    [result appendString:[NSString stringWithFormat:@"%@%@",
                                              [MWMLocationManager speedSymbolFor:location.speed],
                                              @(measurement_utils::FormatSpeedWithDeviceUnits(location.speed).c_str())]];
  }
  return result;
}

static inline NSString * formattedDistance(double const & meters) {
  if (meters < 0.)
    return nil;
  
  auto units = measurement_utils::Units::Metric;
  settings::TryGet(settings::kMeasurementUnits, units);
  
  std::string distance;
  switch (units)
  {
  case measurement_utils::Units::Imperial:
    measurement_utils::FormatDistanceWithLocalization(meters,
                                                      distance,
                                                      [[@" " stringByAppendingString:L(@"mile")] UTF8String],
                                                      [[@" " stringByAppendingString:L(@"foot")] UTF8String]);
  case measurement_utils::Units::Metric:
    measurement_utils::FormatDistanceWithLocalization(meters,
                                                      distance,
                                                      [[@" " stringByAppendingString:L(@"kilometer")] UTF8String],
                                                      [[@" " stringByAppendingString:L(@"meter")] UTF8String]);
  }
  return @(distance.c_str());
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
    north = base::DegToRad(north);
  }
  return north;
}

static inline ms::LatLon ToLatLon(m2::PointD const & p) { return mercator::ToLatLon(p); }

static inline m2::PointD ToMercator(CLLocationCoordinate2D const & l)
{
  return mercator::FromLatLon(l.latitude, l.longitude);
}

static inline m2::PointD ToMercator(ms::LatLon const & l) { return mercator::FromLatLon(l); }
static inline MWMMyPositionMode mwmMyPositionMode(location::EMyPositionMode mode)
{
  switch (mode)
  {
  case location::EMyPositionMode::PendingPosition: return MWMMyPositionModePendingPosition;
  case location::EMyPositionMode::NotFollowNoPosition: return MWMMyPositionModeNotFollowNoPosition;
  case location::EMyPositionMode::NotFollow: return MWMMyPositionModeNotFollow;
  case location::EMyPositionMode::Follow: return MWMMyPositionModeFollow;
  case location::EMyPositionMode::FollowAndRotate: return MWMMyPositionModeFollowAndRotate;
  }
}
}  // namespace MWMLocationHelpers
