#include "location_service.hpp"

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

public:
  AppleLocationService(LocationObserver & observer) : LocationService(observer)
  {
    m_objCppWrapper = [[LocationManagerWrapper alloc] initWithService:this];
    m_locationManager = [[CLLocationManager alloc] init];
    m_locationManager.delegate = m_objCppWrapper;
    m_locationManager.desiredAccuracy = kCLLocationAccuracyBest;
    m_locationManager.purpose = @"Location services are needed to display your current position on the map.";
  }

  virtual ~AppleLocationService()
  {
    [m_locationManager release];
    [m_objCppWrapper release];
  }

  void OnLocationUpdate(GpsInfo const & info)
  {
    m_observer.OnGpsUpdated(info);
  }

  void OnDeniedError()
  {
    m_observer.OnLocationStatusChanged(location::EDisabledByUser);
  }

  virtual void Start()
  {
    if (![CLLocationManager locationServicesEnabled])
    {
      // @TODO correctly handle situation in GUI when wifi is working and native is disabled
      // m_observer.OnLocationStatusChanged(location::ENotSupported);
    }
    else
    {
      switch([CLLocationManager authorizationStatus])
      {
      case kCLAuthorizationStatusAuthorized:
      case kCLAuthorizationStatusNotDetermined:
        [m_locationManager startUpdatingLocation];
        m_observer.OnLocationStatusChanged(location::EStarted);
        break;
      case kCLAuthorizationStatusRestricted:
      case kCLAuthorizationStatusDenied:
        // @TODO correctly handle situation in GUI when wifi is working and native is disabled
        //m_observer.OnLocationStatusChanged(location::EDisabledByUser);
        break;
      }
    }
  }

  virtual void Stop()
  {
    [m_locationManager stopUpdatingLocation];
    m_observer.OnLocationStatusChanged(location::EStopped);
  }
};

@implementation LocationManagerWrapper

- (id)initWithService:(AppleLocationService *) service
{
  if (self = [super init])
    m_service = service;
  return self;
}

+ (void)location:(CLLocation *)location toGpsInfo:(GpsInfo &) info
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

- (void)locationManager:(CLLocationManager *)manager
    didUpdateToLocation:(CLLocation *)newLocation
           fromLocation:(CLLocation *)oldLocation
{
  GpsInfo newInfo;
  [LocationManagerWrapper location:newLocation toGpsInfo:newInfo];
  m_service->OnLocationUpdate(newInfo);
}

- (void)locationManager:(CLLocationManager *)manager
       didFailWithError:(NSError *)error
{
  NSLog(@"locationManager failed with error: %ld, %@", error.code, error.description);
  if (error.code == kCLErrorDenied)
    m_service->OnDeniedError();
}

@end

extern "C" location::LocationService * CreateAppleLocationService(LocationObserver & observer)
{
  return new AppleLocationService(observer);
}
