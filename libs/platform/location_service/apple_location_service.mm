#include "platform/location_service/location_service.hpp"
#include "platform/location_service/position_filter.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

#include "std/target_os.hpp"

#import <CoreLocation/CoreLocation.h>

class AppleLocationService;

@interface LocationManagerWrapper : NSObject <CLLocationManagerDelegate>
{
@private
  AppleLocationService * m_service;
}
- (id)initWithService:(AppleLocationService *)service;
@end

using namespace location;

class AppleLocationService : public LocationService
{
  LocationManagerWrapper * m_objCppWrapper;
  CLLocationManager * m_locationManager;
  PositionFilter m_filter;

public:
  AppleLocationService(LocationObserver & observer) : LocationService(observer)
  {
    m_objCppWrapper = [[LocationManagerWrapper alloc] initWithService:this];
    m_locationManager = [[CLLocationManager alloc] init];
    m_locationManager.delegate = m_objCppWrapper;
    m_locationManager.desiredAccuracy = kCLLocationAccuracyBest;
  }

  void OnLocationUpdate(GpsInfo const & info)
  {
    if (m_filter.Passes(info))
      m_observer.OnLocationUpdated(info);
  }

  void OnDeniedError() { m_observer.OnLocationError(location::EDenied); }

  virtual void Start()
  {
    if (![CLLocationManager locationServicesEnabled])
    {
      // TODO: Report disabled location services to the observer.
    }
    else
    {
      [m_locationManager startUpdatingLocation];
    }
  }

  virtual void Stop() { [m_locationManager stopUpdatingLocation]; }
};

@implementation LocationManagerWrapper

- (id)initWithService:(AppleLocationService *)service
{
  if ((self = [super init]))
    m_service = service;
  return self;
}

+ (void)location:(CLLocation *)location toGpsInfo:(GpsInfo &)info
{
  info.m_horizontalAccuracy = location.horizontalAccuracy;
  info.m_latitude = location.coordinate.latitude;
  info.m_longitude = location.coordinate.longitude;
  info.m_timestamp = [location.timestamp timeIntervalSince1970];
  info.m_source = location::EAppleNative;
}

- (void)locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray<CLLocation *> *)locations
{
  UNUSED_VALUE(manager);
  GpsInfo newInfo;
  [LocationManagerWrapper location:locations.firstObject toGpsInfo:newInfo];
  m_service->OnLocationUpdate(newInfo);
}

- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error
{
  UNUSED_VALUE(manager);
  LOG(LWARNING, ("locationManager failed with error", error.code, error.description.UTF8String));

  if (error.code == kCLErrorDenied)
    m_service->OnDeniedError();
}

@end

std::unique_ptr<location::LocationService> CreateAppleLocationService(LocationObserver & observer)
{
  return std::make_unique<AppleLocationService>(observer);
}
