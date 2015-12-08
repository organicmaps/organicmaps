#import "MWMWatchLocationTracker.h"

#include "geometry/mercator.hpp"
#include "platform/measurement_utils.hpp"

static CLLocationDistance const kNoMovementThreshold = 5.0;
static CLLocationDistance const kDistanceFilter = 5.0;

static CLLocationDistance const kDistanceToDestinationThreshold = 5.0;

@interface MWMWatchLocationTracker () <CLLocationManagerDelegate>

@property (nonatomic, readwrite) CLLocationManager * locationManager;
@property (nonatomic) m2::PointD lastPosition;
@property (nonatomic, readwrite) BOOL hasLocation;
@property (nonatomic, readwrite) BOOL hasDestination;

@end

@implementation MWMWatchLocationTracker

+ (instancetype)sharedLocationTracker
{
  static dispatch_once_t onceToken = 0;
  static MWMWatchLocationTracker *tracker = nil;
  dispatch_once(&onceToken,
  ^{
    tracker = [[super alloc] initUniqueInstance];
  });
  return tracker;
}

- (instancetype)initUniqueInstance
{
  self = [super init];
  if (self)
  {
    self.locationManager = [[CLLocationManager alloc] init];
    [self.locationManager requestAlwaysAuthorization];
    self.locationManager.delegate = self;
    self.locationManager.desiredAccuracy = kCLLocationAccuracyBestForNavigation;
    self.locationManager.activityType = CLActivityTypeFitness;
    self.locationManager.distanceFilter = kDistanceFilter;
    [self.locationManager startUpdatingLocation];
    CLLocationCoordinate2D coordinate = self.currentCoordinate;
    self.lastPosition = MercatorBounds::FromLatLon(coordinate.latitude, coordinate.longitude);
    self.hasLocation = YES;
    self.hasDestination = NO;
  }
  return self;
}

- (location::GpsInfo)infoFromCurrentLocation
{
  location::GpsInfo info;
  info.m_source = location::EAppleNative;

  CLLocation *location = self.locationManager.location;
  CLLocationCoordinate2D currentCoordinate = location.coordinate;
  info.m_latitude = currentCoordinate.latitude;
  info.m_longitude = currentCoordinate.longitude;
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

  return info;
}

- (NSString *)distanceToPoint:(m2::PointD const &)point
{
  CLLocationCoordinate2D coordinate = self.currentCoordinate;
  double const d = MercatorBounds::DistanceOnEarth(point, MercatorBounds::FromLatLon(coordinate.latitude, coordinate.longitude));

  string distance;
  if (MeasurementUtils::FormatDistance(d, distance))
    return @(distance.c_str());
  return @"";
}

- (BOOL)locationServiceEnabled
{
  CLAuthorizationStatus status = [CLLocationManager  authorizationStatus];
  BOOL const haveAuthorization = !(status == kCLAuthorizationStatusNotDetermined || status == kCLAuthorizationStatusRestricted || status == kCLAuthorizationStatusDenied);
  return haveAuthorization;
}

- (CLLocationCoordinate2D)currentCoordinate
{
  return self.locationManager.location.coordinate;
}

- (BOOL)arrivedToDestination
{
  if (self.hasDestination)
  {
    CLLocationCoordinate2D coordinate = self.currentCoordinate;
    m2::PointD currentPosition (MercatorBounds::FromLatLon(coordinate.latitude, coordinate.longitude));
    double const distance = MercatorBounds::DistanceOnEarth(self.destinationPosition, currentPosition);
    return distance <= kDistanceToDestinationThreshold;
  }
  return NO;
}

- (BOOL)movementStopped
{
  CLLocationCoordinate2D coordinate = self.currentCoordinate;
  m2::PointD currentPosition (MercatorBounds::FromLatLon(coordinate.latitude, coordinate.longitude));

  double const distance = MercatorBounds::DistanceOnEarth(self.lastPosition, currentPosition);
  self.lastPosition = currentPosition;
  return distance < kNoMovementThreshold;
}

- (void)setDestinationPosition:(m2::PointD)destinationPosition
{
  _destinationPosition = destinationPosition;
  self.hasDestination = YES;
}

- (void)clearDestination
{
  self.hasDestination = NO;
}

#pragma mark - CLLocationManagerDelegate

- (void)locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
  self.hasLocation = YES;
  if ([self.delegate respondsToSelector:@selector(onChangeLocation:)])
    [self.delegate onChangeLocation:locations.lastObject];
}

- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error
{
  self.hasLocation = NO;
  if ([self.delegate respondsToSelector:@selector(locationTrackingFailedWithError:)])
    [self.delegate locationTrackingFailedWithError:error];
}

- (void)locationManager:(CLLocationManager *)manager didChangeAuthorizationStatus:(CLAuthorizationStatus)status
{
  if ([self.delegate respondsToSelector:@selector(didChangeAuthorizationStatus:)])
    [self.delegate didChangeAuthorizationStatus:self.locationServiceEnabled];
}

@end
