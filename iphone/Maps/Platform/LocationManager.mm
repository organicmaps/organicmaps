#import "LocationManager.h"

#include "../../map/measurement_utils.hpp"

#include "../../platform/settings.hpp"

#include "../../base/math.hpp"


@implementation LocationManager

- (id)init
{
  if ((self = [super init]))
  {
    m_locationManager = [[CLLocationManager alloc] init];
    m_locationManager.delegate = self;
    m_locationManager.purpose = NSLocalizedString(@"location_services_are_needed_desc", @"Location purpose text description");
    m_locationManager.desiredAccuracy = kCLLocationAccuracyBest;
    m_locationManager.headingFilter = 3.0;
    m_locationManager.distanceFilter = 3.0;
    m_isStarted = NO;
    m_observers = [[NSMutableSet alloc] init];

    m_lastLocationTime = nil;
    m_isCourse = NO;
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientationChanged) name:UIDeviceOrientationDidChangeNotification  object:nil];
  }
  return self;
}

- (void)dealloc
{
  [m_observers release];
  [m_locationManager release];
  [m_lastLocationTime release];
  [[NSNotificationCenter defaultCenter] removeObserver:self];

  [super dealloc];
}

- (void)start:(id <LocationObserver>)observer
{
  if (!m_isStarted)
  {
    if ([CLLocationManager locationServicesEnabled])
    {
      CLAuthorizationStatus authStatus = kCLAuthorizationStatusNotDetermined;
      // authorizationStatus method is implemented in iOS >= 4.2
      if ([CLLocationManager respondsToSelector:@selector(authorizationStatus)])
        authStatus = [CLLocationManager authorizationStatus];

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
      [observer onLocationError:location::ENotSupported];
  }
  else
  {
    [m_observers addObject:observer];
    if ([self lastLocationIsValid])
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
  return m_locationManager.location;
}

- (CLHeading *)lastHeading
{
  return m_locationManager.heading;
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
  [m_lastLocationTime release];
  m_lastLocationTime = [[NSDate date] retain];

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

- (bool)lastLocationIsValid
{
    return (([self lastLocation] != nil) && ([m_lastLocationTime timeIntervalSinceNow] > -300.0));
}

-(void)orientationChanged
{
  m_locationManager.headingOrientation = (CLDeviceOrientation)[UIDevice currentDevice].orientation;
}

-(void)notifyCompassUpdate:(location::CompassInfo const &)newInfo
{
  for (id observer in m_observers)
    if ([observer respondsToSelector:@selector(onCompassUpdate:)])
      [observer onCompassUpdate:newInfo];
}

@end
