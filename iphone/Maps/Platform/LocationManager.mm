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
    m_reportFirstUpdate = YES;
    m_observers = [[NSMutableSet alloc] init];
  }
  return self;
}

- (void)dealloc
{
  [m_observers release];
  [m_locationManager release];
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

      switch(authStatus)
      {
      case kCLAuthorizationStatusAuthorized:
      case kCLAuthorizationStatusNotDetermined:
        [m_locationManager startUpdatingLocation];
        if ([CLLocationManager headingAvailable])
          [m_locationManager startUpdatingHeading];
        m_isStarted = YES;
        [m_observers addObject:observer];
        [observer onLocationStatusChanged:location::EStarted];
        break;
      case kCLAuthorizationStatusRestricted:
      case kCLAuthorizationStatusDenied:
        [observer onLocationStatusChanged:location::EDisabledByUser];
        break;
      }
    }
    else
      [observer onLocationStatusChanged:location::ENotSupported];
  }
  else
  {
    [m_observers addObject:observer];
    [observer onLocationStatusChanged:location::EStarted];
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
      m_reportFirstUpdate = YES;
      if ([CLLocationManager headingAvailable])
        [m_locationManager stopUpdatingHeading];
      [m_locationManager stopUpdatingLocation];
    }
  }
  [observer onLocationStatusChanged:location::EStopped];
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

  //info.m_verticalAccuracy = location.verticalAccuracy;
  //info.m_altitude = location.altitude;
  //info.m_course = location.course;
  //info.m_speed = location.speed;
}

- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation
{
  if (location::IsLatValid(newLocation.coordinate.latitude) &&
      location::IsLonValid(newLocation.coordinate.longitude))
  {
    if (m_reportFirstUpdate)
    {
      for (id observer in m_observers)
        [observer onLocationStatusChanged:location::EFirstEvent];
      m_reportFirstUpdate = NO;
    }
  
    location::GpsInfo newInfo;
    [self location:newLocation toGpsInfo:newInfo];
    for (id observer in m_observers)
      [observer onGpsUpdate:newInfo];
  }
}

- (void)locationManager:(CLLocationManager *)manager didUpdateHeading:(CLHeading *)newHeading
{
  location::CompassInfo newInfo;
  newInfo.m_magneticHeading = my::DegToRad(newHeading.magneticHeading);
  newInfo.m_trueHeading = my::DegToRad(newHeading.trueHeading);
  newInfo.m_accuracy = my::DegToRad(newHeading.headingAccuracy);

  newInfo.m_timestamp = [newHeading.timestamp timeIntervalSince1970];

  for (id observer in m_observers)
    [observer onCompassUpdate:newInfo];
}

- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error
{
  NSLog(@"locationManager failed with error: %d, %@", error.code, error.description);
  if (error.code == kCLErrorDenied)
  {
    for (id observer in m_observers)
      [observer onLocationStatusChanged:location::EDisabledByUser];
  }
}

- (void)onTimer:(NSTimer *)timer
{
  [m_locationManager dismissHeadingCalibrationDisplay];
  m_isTimerActive = NO;
}

// Display compass calibration dialog automatically
- (BOOL)locationManagerShouldDisplayHeadingCalibration:(CLLocationManager *)manager
{
  if (!m_isTimerActive)
  {
    [NSTimer scheduledTimerWithTimeInterval:2.5 target:self selector:@selector(onTimer:)
        userInfo:nil repeats:NO];
    m_isTimerActive = YES;
  }
  return YES;
}

- (void)setOrientation:(UIInterfaceOrientation)orientation
{
  m_locationManager.headingOrientation = (CLDeviceOrientation)orientation;
}

- (bool)getLat:(double &)lat Lon:(double &)lon
{
  CLLocation * l = [self lastLocation];

  static NSTimeInterval const SECONDS_TO_EXPIRE = 300.0;

  // timeIntervalSinceNow returns negative value - because of "since now"
  if ((l != nil) && ([l.timestamp timeIntervalSinceNow] > (-SECONDS_TO_EXPIRE)))
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

@end
