#include "location.hpp"

#include "../std/target_os.hpp"

#import <CoreLocation/CoreLocation.h>

class AppleLocationService;

@interface LocationManagerWrapper : NSObject <CLLocationManagerDelegate> {
@private
  AppleLocationService * m_service;
}
- (id)initWithService:(AppleLocationService *) service;
@end

using namespace location;

#define ROUGH_ACCURACY kCLLocationAccuracyNearestTenMeters

class AppleLocationService : public LocationService
{
  LocationManagerWrapper * m_objCppWrapper;
  CLLocationManager * m_locationManager;

  TLocationStatus m_status;

public:
  AppleLocationService() : m_status(ENotSupported)
  {
    m_objCppWrapper = [[LocationManagerWrapper alloc] initWithService:this];
    m_locationManager = [[CLLocationManager alloc] init];
    m_locationManager.delegate = m_objCppWrapper;
  }

  ~AppleLocationService()
  {
    [m_locationManager release];
    [m_objCppWrapper release];
  }

  void OnLocationUpdate(GpsInfo & newLocation)
  {
    newLocation.m_status = m_status;
    NotifyGpsObserver(newLocation);
  }

  void OnHeadingUpdate(CompassInfo & newHeading)
  {
    NotifyCompassObserver(newHeading);
  }

//  virtual bool IsServiceSupported()
//  {
//    // Mac OS 10.6+ and iOS 4.0+ support this definitely
//    return true;
//  }

//  virtual bool IsServiceEnabled()
//  {
//    return [CLLocationManager locationServicesEnabled];
//  }

//  virtual bool IsCompassAvailable()
//  {
//#ifdef OMIM_OS_MAC
//      return false;
//#else // iOS 4.0+ have it
//      return [CLLocationManager headingAvailable];
//#endif
//  }

  virtual void StartUpdate(bool useAccurateMode)
  {
    if (![CLLocationManager locationServicesEnabled])
    {
      m_status = EDisabledByUser;
      GpsInfo info;
      info.m_status = m_status;
      NotifyGpsObserver(info);
    }
    else
    {
      if (useAccurateMode)
      {
        m_status = EAccurateMode;
        m_locationManager.desiredAccuracy = kCLLocationAccuracyBest;
      }
      else
      {
        m_status = ERoughMode;
        m_locationManager.desiredAccuracy = ROUGH_ACCURACY;
      }
      [m_locationManager startUpdatingLocation];
      // enable compass
#ifdef OMIM_OS_IPHONE
      if ([CLLocationManager headingAvailable])
      {
        m_locationManager.headingFilter = 1.0;
        [m_locationManager startUpdatingHeading];
      }
#endif
    }
  }

  virtual void StopUpdate()
  {
#ifdef OMIM_OS_IPHONE
    if ([CLLocationManager headingAvailable])
      [m_locationManager stopUpdatingHeading];
#endif
    [m_locationManager stopUpdatingLocation];
  }
};

@implementation LocationManagerWrapper

- (id)initWithService:(AppleLocationService *) service
{
  self = [super init];
  if (self) {
    m_service = service;
  }
  return self;
}

- (void)dealloc
{
  [super dealloc];
}

+ (void)location:(CLLocation *)location toGpsInfo:(GpsInfo &) info
{
  info.m_altitude = location.altitude;
  info.m_course = location.course;
  info.m_speed = location.speed;
  info.m_horizontalAccuracy = location.horizontalAccuracy;
  info.m_verticalAccuracy = location.verticalAccuracy;
  info.m_latitude = location.coordinate.latitude;
  info.m_longitude = location.coordinate.longitude;
  info.m_timestamp = [location.timestamp timeIntervalSince1970];
  info.m_source = location::EAppleNative;
}

- (void)locationManager:(CLLocationManager *)manager
    didUpdateToLocation:(CLLocation *)newLocation
           fromLocation:(CLLocation *)oldLocation
{
  GpsInfo newInfo;
  [LocationManagerWrapper location:newLocation toGpsInfo:newInfo];
  m_service->OnLocationUpdate(newInfo);
}

#ifdef OMIM_OS_IPHONE
- (void)locationManager:(CLLocationManager *)manager
       didUpdateHeading:(CLHeading *)newHeading
{
  CompassInfo newInfo;
  newInfo.m_magneticHeading = newHeading.magneticHeading;
  newInfo.m_trueHeading = newHeading.trueHeading;
  newInfo.m_accuracy = newHeading.headingAccuracy;
  newInfo.m_x = newHeading.x;
  newInfo.m_y = newHeading.y;
  newInfo.m_z = newHeading.z;
  newInfo.m_timestamp = [newHeading.timestamp timeIntervalSince1970];
  m_service->OnHeadingUpdate(newInfo);
}
#endif

- (void)locationManager:(CLLocationManager *)manager
       didFailWithError:(NSError *)error
{
  NSLog(@"locationManager failed with error: %ld, %@", error.code, error.description);
  if (error.code == kCLErrorDenied)
  {
    GpsInfo info;
    info.m_status = EDisabledByUser;
    info.m_source = location::EAppleNative;
    info.m_timestamp = [[NSDate date] timeIntervalSince1970];
    m_service->OnLocationUpdate(info);
  }
}

// Display compass calibration dialog automatically
- (BOOL)locationManagerShouldDisplayHeadingCalibration:(CLLocationManager *)manager
{
  return YES;
}

@end

location::LocationService * CreateAppleLocationService()
{
  return new AppleLocationService();
}
