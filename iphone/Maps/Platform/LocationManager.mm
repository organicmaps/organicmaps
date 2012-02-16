#import "LocationManager.h"

@implementation LocationManager

- (id)init
{
  if (self = [super init])
  {
    m_locationManager = [[CLLocationManager alloc] init];
    m_locationManager.delegate = self;
    m_locationManager.purpose = NSLocalizedString(@"Location Services are needed to display your current position on the map.", @"Location purpose text description");
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
    { // stop only if no more observers are subsribed
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
//  info.m_verticalAccuracy = location.verticalAccuracy;
//  info.m_altitude = location.altitude;
//  info.m_course = location.course;
//  info.m_speed = location.speed;
}

- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation
{
  if (location::IsLatValid(newLocation.coordinate.latitude)
    && location::IsLonValid(newLocation.coordinate.longitude))
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
  newInfo.m_magneticHeading = newHeading.magneticHeading;
  newInfo.m_trueHeading = newHeading.trueHeading;
  newInfo.m_accuracy = newHeading.headingAccuracy;
//  newInfo.m_x = newHeading.x;
//  newInfo.m_y = newHeading.y;
//  newInfo.m_z = newHeading.z;
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

@end
