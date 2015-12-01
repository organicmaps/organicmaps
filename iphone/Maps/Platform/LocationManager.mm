#import "Common.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMMapViewControlsManager.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"
#include "base/math.hpp"

static CLAuthorizationStatus const kRequestAuthStatus = kCLAuthorizationStatusAuthorizedAlways;
static NSString * const kAlohalyticsLocationRequestAlwaysFailed = @"$locationAlwaysRequestErrorDenied";

@implementation LocationManager

- (id)init
{
  if ((self = [super init]))
  {
    m_locationManager = [[CLLocationManager alloc] init];
    m_locationManager.delegate = self;
    [UIDevice currentDevice].batteryMonitoringEnabled = YES;
    [self refreshAccuracy];
    if (!isIOSVersionLessThan(9))
      m_locationManager.allowsBackgroundLocationUpdates = YES;
    m_locationManager.pausesLocationUpdatesAutomatically = YES;
    m_locationManager.headingFilter = 3.0;
    //    m_locationManager.distanceFilter = 3.0;
    m_isStarted = NO;
    m_observers = [[NSMutableSet alloc] init];
    m_lastLocationTime = nil;

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientationChanged) name:UIDeviceOrientationDidChangeNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(batteryStateChangedNotification:) name:UIDeviceBatteryStateDidChangeNotification object:nil];
  }
  return self;
}

- (void)batteryStateChangedNotification:(NSNotification *)notification
{
  [self refreshAccuracy];
}

- (void)refreshAccuracy
{
  UIDeviceBatteryState state = [UIDevice currentDevice].batteryState;
  m_locationManager.desiredAccuracy = (state == UIDeviceBatteryStateCharging) ? kCLLocationAccuracyBestForNavigation : kCLLocationAccuracyBest;
}

- (void)dealloc
{
  m_locationManager.delegate = nil;
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
        case kCLAuthorizationStatusAuthorizedWhenInUse:
        case kCLAuthorizationStatusAuthorizedAlways:
        case kCLAuthorizationStatusNotDetermined:
          if (kRequestAuthStatus == kCLAuthorizationStatusAuthorizedAlways && [m_locationManager respondsToSelector:@selector(requestAlwaysAuthorization)])
            [m_locationManager requestAlwaysAuthorization];
          [m_locationManager startUpdatingLocation];
          if ([CLLocationManager headingAvailable])
            [m_locationManager startUpdatingHeading];
          m_isStarted = YES;
          [m_observers addObject:observer];
          break;
        case kCLAuthorizationStatusRestricted:
        case kCLAuthorizationStatusDenied:
          [self observer:observer onLocationError:location::EDenied];
          break;
      }
    }
    else
      [self observer:observer onLocationError:location::EDenied];
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
  info.m_source = location::EAppleNative;

  info.m_latitude = location.coordinate.latitude;
  info.m_longitude = location.coordinate.longitude;
  info.m_timestamp = [location.timestamp timeIntervalSince1970];

  if (location.horizontalAccuracy >= 0.0)
    info.m_horizontalAccuracy = location.horizontalAccuracy;

  if (location.verticalAccuracy >= 0.0)
  {
    info.m_verticalAccuracy = location.verticalAccuracy;
    info.m_altitude = location.altitude;
  }

  if (location.course >= 0.0)
    info.m_bearing = location.course;

  if (location.speed >= 0.0)
    info.m_speed = location.speed;
}

- (void)heading:(CLHeading *)heading toCompassInfo:(location::CompassInfo &)info
{
  if (heading.trueHeading >= 0.0)
    info.m_bearing = my::DegToRad(heading.trueHeading);
  else if (heading.headingAccuracy >= 0.0)
    info.m_bearing = my::DegToRad(heading.magneticHeading);
}

- (void)triggerCompass
{
  [self locationManager:m_locationManager didUpdateHeading:m_locationManager.heading];
}

- (void)locationManager:(CLLocationManager *)manager didUpdateHeading:(CLHeading *)heading
{
  location::CompassInfo info;
  [self heading:heading toCompassInfo:info];
  [self notifyCompassUpdate:info];
}

- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation
{
  [self processLocation:newLocation];
}

- (void)locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
  CLLocation * newLocation = [locations lastObject];
  [self processLocation:newLocation];
}

- (void)processLocation:(CLLocation *)newLocation
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
  // TODO(AlexZ): Temporary, remove in the future.
  [[Statistics instance] logLocation:newLocation];
}

- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error
{
  NSLog(@"locationManager failed with error: %ld, %@", (long)error.code, error.description);
  if (error.code == kCLErrorDenied)
  {
    if (kRequestAuthStatus == kCLAuthorizationStatusAuthorizedAlways && [m_locationManager respondsToSelector:@selector(requestAlwaysAuthorization)])
      [Alohalytics logEvent:kAlohalyticsLocationRequestAlwaysFailed];
    for (id observer in m_observers)
      [self observer:observer onLocationError:location::EDenied];
  }
}

- (BOOL)locationManagerShouldDisplayHeadingCalibration:(CLLocationManager *)manager
{
  bool on = false;
  Settings::Get("CompassCalibrationEnabled", on);
  if (!on)
    return NO;

  if (!MapsAppDelegate.theApp.mapViewController.controlsManager.searchHidden)
    return NO;
  if (!manager.heading)
    return YES;
  else if (manager.heading.headingAccuracy < 0)
    return YES;
  else if (manager.heading.headingAccuracy > 5)
    return YES;
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

+ (NSString *)formattedDistance:(double)meters
{
  if (meters < 0.)
    return nil;

  string s;
  MeasurementUtils::FormatDistance(meters, s);
  return @(s.c_str());
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
  static char const * airplane = "\xE2\x9C\x88\xEF\xB8\x8F ";
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

- (NSString *)formattedSpeedAndAltitude:(BOOL &)hasSpeed
{
  hasSpeed = NO;
  CLLocation * l = [self lastLocation];
  if (l)
  {
    string result;
    if (l.altitude)
      result = "\xE2\x96\xB2 " /* this is simple mountain symbol */ + MeasurementUtils::FormatAltitude(l.altitude);
    // Speed is actual only for just received location
    if (l.speed > 0. && [l.timestamp timeIntervalSinceNow] >= -2.0)
    {
      hasSpeed = YES;
      if (!result.empty())
        result += "   ";
      result += [LocationManager getSpeedSymbol:l.speed] + MeasurementUtils::FormatSpeed(l.speed);
    }
    return result.empty() ? nil : @(result.c_str());
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

- (void)observer:(id<LocationObserver>)observer onLocationError:(location::TLocationError)errorCode
{
  if ([(NSObject *)observer respondsToSelector:@selector(onLocationError:)])
    [observer onLocationError:errorCode];
}

@end

@implementation CLLocation (Mercator)

- (m2::PointD)mercator
{
  return ToMercator(self.coordinate);
}

@end
