#import "LocationManager.h"

#include "../../map/measurement_utils.hpp"

#include "../../platform/settings.hpp"

#include "../../base/math.hpp"
#import "MapViewController.h"

@implementation LocationManager

- (id)init
{
  if ((self = [super init]))
  {
    m_locationManager = [[CLLocationManager alloc] init];
    m_locationManager.delegate = self;
    m_locationManager.purpose = NSLocalizedString(@"location_services_are_needed_desc", @"Location purpose text description");
    m_locationManager.desiredAccuracy = kCLLocationAccuracyBest;
//    m_locationManager.headingFilter = 3.0;
//    m_locationManager.distanceFilter = 3.0;
    m_isStarted = NO;
    m_observers = [[NSMutableSet alloc] init];

    m_lastLocationTime = nil;
    m_isCourse = NO;
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientationChanged) name:UIDeviceOrientationDidChangeNotification object:nil];
  }
  return self;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)start:(id <LocationObserver>)observer
{
  if (!m_isStarted)
  {
    //YES if location services are enabled; NO if they are not.
    if ([CLLocationManager locationServicesEnabled])
    {
      CLAuthorizationStatus const authStatus = [CLLocationManager authorizationStatus];
      switch (authStatus)
      {
        case kCLAuthorizationStatusAuthorized:
        case kCLAuthorizationStatusNotDetermined:
          [m_locationManager startUpdatingLocation];
          if ([CLLocationManager headingAvailable])
            [m_locationManager startUpdatingHeading];
          m_isStarted = YES;
          [m_observers addObject:observer];
          break;
        case kCLAuthorizationStatusRestricted:
        case kCLAuthorizationStatusDenied:
          [observer onLocationError:location::EDenied];
          break;
      }
    }
    else
      [observer onLocationError:location::EDenied];
  }
  else
  {
    BOOL updateLocation = ![m_observers containsObject:observer];
    [m_observers addObject:observer];
    if ([self lastLocationIsValid] && updateLocation)
    {
      // pass last location known location when new observer is attached
      // (default CLLocationManagerDelegate behaviour)
      location::GpsInfo newInfo;
      [self location:[self lastLocation] toGpsInfo:newInfo];
      [observer onLocationUpdate:newInfo];
    }
  }
}

- (void)stop:(id <LocationObserver>)observer
{
  [m_observers removeObject:observer];
  if (m_isStarted)
  {
    if ([m_observers count] == 0)
    {
      // stop only if no more observers are subsribed
      m_isStarted = NO;
      if ([CLLocationManager headingAvailable])
        [m_locationManager stopUpdatingHeading];
      [m_locationManager stopUpdatingLocation];
    }
  }
}

- (CLLocation *)lastLocation
{
  return m_isStarted ? m_locationManager.location : nil;
}

- (CLHeading *)lastHeading
{
  return m_isStarted ? m_locationManager.heading : nil;
}

- (void)location:(CLLocation *)location toGpsInfo:(location::GpsInfo &)info
{
  info.m_horizontalAccuracy = location.horizontalAccuracy;
  info.m_latitude = location.coordinate.latitude;
  info.m_longitude = location.coordinate.longitude;
  info.m_timestamp = [location.timestamp timeIntervalSince1970];
  info.m_source = location::EAppleNative;

  info.m_verticalAccuracy = location.verticalAccuracy;
  info.m_altitude = location.altitude;
  info.m_course = location.course;
  info.m_speed = location.speed;
}

- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation
{
  // According to documentation, lat and lon are valid only if horz acc is non-negative.
  // So we filter out such events completely.
  if (newLocation.horizontalAccuracy < 0.)
    return;

  // Save current device time for location.
  m_lastLocationTime = [NSDate date];

  location::GpsInfo newInfo;
  [self location:newLocation toGpsInfo:newInfo];
  for (id observer in m_observers)
    [observer onLocationUpdate:newInfo];

  // Pass current course if we are moving and GPS course is valid.
  if (newLocation.speed >= 1.0 && newLocation.course >= 0.0)
  {
    m_isCourse = YES;

    location::CompassInfo newInfo;
    newInfo.m_magneticHeading = my::DegToRad(newLocation.course);
    newInfo.m_trueHeading = newInfo.m_magneticHeading;
    newInfo.m_accuracy = 10.0;
    newInfo.m_timestamp = [newLocation.timestamp timeIntervalSince1970];

    [self notifyCompassUpdate:newInfo];
  }
  else
    m_isCourse = NO;
}

- (void)locationManager:(CLLocationManager *)manager didUpdateHeading:(CLHeading *)newHeading
{
  // Stop passing driving course if last time stamp for GPS location is later than 20 seconds.
  if (m_lastLocationTime == nil || ([m_lastLocationTime timeIntervalSinceNow] < -20.0))
    m_isCourse = NO;

  if (!m_isCourse)
  {
    location::CompassInfo newInfo;
    newInfo.m_magneticHeading = my::DegToRad(newHeading.magneticHeading);
    newInfo.m_trueHeading = my::DegToRad(newHeading.trueHeading);
    newInfo.m_accuracy = my::DegToRad(newHeading.headingAccuracy);
    newInfo.m_timestamp = [newHeading.timestamp timeIntervalSince1970];

    [self notifyCompassUpdate:newInfo];
  }
}

- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error
{
  NSLog(@"locationManager failed with error: %d, %@", error.code, error.description);
  if (error.code == kCLErrorDenied)
  {
    for (id observer in m_observers)
      [observer onLocationError:location::EDenied];
  }
}

- (BOOL)locationManagerShouldDisplayHeadingCalibration:(CLLocationManager *)manager
{
  // Never display calibration dialog as it sucks on iOS 7
  return NO;
}

- (bool)getLat:(double &)lat Lon:(double &)lon
{
  CLLocation * l = [self lastLocation];

  // Return last saved location if it's not later than 5 minutes.
  if ([self lastLocationIsValid])
  {
    lat = l.coordinate.latitude;
    lon = l.coordinate.longitude;
    return true;
  }

  return false;
}

- (bool)getNorthRad:(double &)rad
{
  CLHeading * h = [self lastHeading];

  if (h != nil)
  {
    rad = (h.trueHeading < 0) ? h.magneticHeading : h.trueHeading;
    rad = my::DegToRad(rad);
    return true;
  }

  return false;
}

+ (NSString *)formatDistance:(double)meters
{
  if (meters < 0.)
    return nil;

  string s;
  MeasurementUtils::FormatDistance(meters, s);
  return [NSString stringWithUTF8String:s.c_str()];
}

+ (char const *)getSpeedSymbol:(double)metersPerSecond
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
  static char const * airplane = "\xF0\x9F\x9B\xA7 ";
  // 278+
  static char const * rocket = "\xF0\x9F\x9A\x80 ";

  if (metersPerSecond <= 1.) return turtle;
  else if (metersPerSecond <= 2.) return pedestrian;
  else if (metersPerSecond <= 5.) return tractor;
  else if (metersPerSecond <= 10.) return bicycle;
  else if (metersPerSecond <= 36.) return car;
  else if (metersPerSecond <= 120.) return train;
  else if (metersPerSecond <= 278.) return airplane;
  else return rocket;
}

- (NSString *)formatSpeedAndAltitude
{
  CLLocation * l = [self lastLocation];
  if (l)
  {
    string result;
    if (l.altitude)
      result = "\xE2\x96\xB2 " /* this is simple mountain symbol */ + MeasurementUtils::FormatAltitude(l.altitude);
    // Speed is actual only for just received location
    if (l.speed > 0. && [l.timestamp timeIntervalSinceNow] >= -2.0)
    {
      if (!result.empty())
        result += "   ";
      result += [LocationManager getSpeedSymbol:l.speed] + MeasurementUtils::FormatSpeed(l.speed);
    }
    return result.empty() ? nil : [NSString stringWithUTF8String:result.c_str()];
  }
  return nil;
}

- (bool)lastLocationIsValid
{
  return (([self lastLocation] != nil) && ([m_lastLocationTime timeIntervalSinceNow] > -300.0));
}

- (BOOL)enabledOnMap
{
  for (id observer in m_observers)
    if ([observer isKindOfClass:[MapViewController class]])
      return YES;
  return NO;
}

- (void)orientationChanged
{
  m_locationManager.headingOrientation = (CLDeviceOrientation)[UIDevice currentDevice].orientation;
}

- (void)notifyCompassUpdate:(location::CompassInfo const &)newInfo
{
  for (id observer in m_observers)
    if ([observer respondsToSelector:@selector(onCompassUpdate:)])
      [observer onCompassUpdate:newInfo];
}

@end
