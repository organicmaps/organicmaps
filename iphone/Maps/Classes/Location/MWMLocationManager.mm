#import "MWMLocationManager.h"
#import <Pushwoosh/PushNotificationManager.h>
#import "Common.h"
#import "MWMAlertViewController.h"
#import "MWMController.h"
#import "MWMLocationPredictor.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

#include "map/gps_tracker.hpp"

#include "std/map.hpp"

namespace
{
using TObserver = id<MWMLocationObserver>;
using TObservers = NSHashTable<__kindof TObserver>;

location::GpsInfo gpsInfoFromLocation(CLLocation * l)
{
  location::GpsInfo info;
  info.m_source = location::EAppleNative;

  info.m_latitude = l.coordinate.latitude;
  info.m_longitude = l.coordinate.longitude;
  info.m_timestamp = l.timestamp.timeIntervalSince1970;

  if (l.horizontalAccuracy >= 0.0)
    info.m_horizontalAccuracy = l.horizontalAccuracy;

  if (l.verticalAccuracy >= 0.0)
  {
    info.m_verticalAccuracy = l.verticalAccuracy;
    info.m_altitude = l.altitude;
  }

  if (l.course >= 0.0)
    info.m_bearing = l.course;

  if (l.speed >= 0.0)
    info.m_speed = l.speed;
  return info;
}

location::CompassInfo compassInfoFromHeading(CLHeading * h)
{
  location::CompassInfo info;
  if (h.trueHeading >= 0.0)
    info.m_bearing = my::DegToRad(h.trueHeading);
  else if (h.headingAccuracy >= 0.0)
    info.m_bearing = my::DegToRad(h.magneticHeading);
  return info;
}

enum class GeoMode
{
  Pending,
  InPosition,
  NotInPosition,
  FollowAndRotate,
  VehicleRouting,
  PedestrianRouting,
  BicycleRouting
};

struct DesiredAccuracy
{
  CLLocationAccuracy charging;
  CLLocationAccuracy battery;
};

struct GeoModeSettings
{
  CLLocationDistance distanceFilter;
  DesiredAccuracy accuracy;
};

map<GeoMode, GeoModeSettings> const kGeoSettings{
    {GeoMode::Pending,
     {.distanceFilter = kCLDistanceFilterNone,
      .accuracy = {.charging = kCLLocationAccuracyBestForNavigation,
                   .battery = kCLLocationAccuracyBestForNavigation}}},
    {GeoMode::InPosition,
     {.distanceFilter = 2,
      .accuracy = {.charging = kCLLocationAccuracyBestForNavigation,
                   .battery = kCLLocationAccuracyBest}}},
    {GeoMode::NotInPosition,
     {.distanceFilter = 5,
      .accuracy = {.charging = kCLLocationAccuracyBestForNavigation,
                   .battery = kCLLocationAccuracyBest}}},
    {GeoMode::FollowAndRotate,
     {.distanceFilter = 2,
      .accuracy = {.charging = kCLLocationAccuracyBestForNavigation,
                   .battery = kCLLocationAccuracyBest}}},
    {GeoMode::VehicleRouting,
     {.distanceFilter = kCLDistanceFilterNone,
      .accuracy = {.charging = kCLLocationAccuracyBestForNavigation,
                   .battery = kCLLocationAccuracyBest}}},
    {GeoMode::PedestrianRouting,
     {.distanceFilter = 2,
      .accuracy = {.charging = kCLLocationAccuracyBestForNavigation,
                   .battery = kCLLocationAccuracyBest}}},
    {GeoMode::BicycleRouting,
     {.distanceFilter = 2,
      .accuracy = {.charging = kCLLocationAccuracyBestForNavigation,
                   .battery = kCLLocationAccuracyBest}}}};

BOOL keepRunningInBackground()
{
  bool const needGPSForTrackRecorder = GpsTracker::Instance().IsEnabled();
  if (needGPSForTrackRecorder)
    return YES;

  auto const & f = GetFramework();
  bool const isRouteBuilt = f.IsRouteBuilt();
  bool const isRouteFinished = f.IsRouteFinished();
  bool const isRouteRebuildingOnly = f.IsRouteRebuildingOnly();
  bool const needGPSForRouting = ((isRouteBuilt || isRouteRebuildingOnly) && !isRouteFinished);
  if (needGPSForRouting)
    return YES;

  return NO;
}

void sendInfoToFramework(dispatch_block_t block)
{
  MapsAppDelegate * delegate =
      static_cast<MapsAppDelegate *>(UIApplication.sharedApplication.delegate);
  if (delegate.isDrapeEngineCreated)
  {
    block();
  }
  else
  {
    runAsyncOnMainQueue(^{
      sendInfoToFramework(block);
    });
  }
}

NSString * const kLocationPermissionRequestedKey = @"kLocationPermissionRequestedKey";

BOOL isPermissionRequested()
{
  return [[NSUserDefaults standardUserDefaults] boolForKey:kLocationPermissionRequestedKey];
}

void setPermissionRequested()
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setBool:YES forKey:kLocationPermissionRequestedKey];
  [ud synchronize];
}
}  // namespace

@interface MWMLocationManager ()<CLLocationManagerDelegate>

@property(nonatomic) BOOL started;
@property(nonatomic) CLLocationManager * locationManager;
@property(nonatomic) GeoMode geoMode;
@property(nonatomic) CLHeading * lastHeadingInfo;
@property(nonatomic) CLLocation * lastLocationInfo;
@property(nonatomic) location::TLocationError lastLocationStatus;
@property(nonatomic) MWMLocationPredictor * predictor;
@property(nonatomic) TObservers * observers;

@end

@implementation MWMLocationManager

#pragma mark - Init

+ (MWMLocationManager *)manager
{
  static MWMLocationManager * manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[super alloc] initManager];
  });
  return manager;
}

- (instancetype)initManager
{
  self = [super init];
  if (self)
    _observers = [TObservers weakObjectsHashTable];
  return self;
}

#pragma mark - Add/Remove Observers

+ (void)addObserver:(TObserver)observer
{
  runAsyncOnMainQueue(^{
    MWMLocationManager * manager = [MWMLocationManager manager];
    [manager.observers addObject:observer];
    [manager processLocationUpdate:manager.lastLocationInfo];
  });
}

+ (void)removeObserver:(TObserver)observer
{
  runAsyncOnMainQueue(^{
    [[MWMLocationManager manager].observers removeObject:observer];
  });
}

#pragma mark - App Life Cycle

+ (void)applicationDidBecomeActive
{
  if (isPermissionRequested() || ![Alohalytics isFirstSession])
    [MWMLocationManager manager].started = YES;
}

+ (void)applicationWillResignActive
{
  BOOL const keepRunning = isPermissionRequested() && keepRunningInBackground();
  MWMLocationManager * manager = [MWMLocationManager manager];
  CLLocationManager * locationManager = manager.locationManager;
  if ([locationManager respondsToSelector:@selector(setAllowsBackgroundLocationUpdates:)])
    [locationManager setAllowsBackgroundLocationUpdates:keepRunning];
  manager.started = keepRunning;
}

#pragma mark - Getters

+ (CLLocation *)lastLocation
{
  MWMLocationManager * manager = [MWMLocationManager manager];
  if (!manager.started || !manager.lastLocationInfo ||
      manager.lastLocationInfo.horizontalAccuracy < 0 ||
      manager.lastLocationStatus != location::TLocationError::ENoError)
    return nil;
  return manager.lastLocationInfo;
}

+ (location::TLocationError)lastLocationStatus
{
  return [MWMLocationManager manager].lastLocationStatus;
}

+ (CLHeading *)lastHeading
{
  MWMLocationManager * manager = [MWMLocationManager manager];
  if (!manager.started || !manager.lastHeadingInfo || manager.lastHeadingInfo.headingAccuracy < 0)
    return nil;
  return manager.lastHeadingInfo;
}

#pragma mark - Observer notifications

- (void)processLocationStatus:(location::TLocationError)locationError
{
  //  if (self.lastLocationStatus == locationError)
  //    return;
  self.lastLocationStatus = locationError;
  sendInfoToFramework(^{
    if (self.lastLocationStatus != location::TLocationError::ENoError)
      GetFramework().OnLocationError(self.lastLocationStatus);
  });
  for (TObserver observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onLocationError:)])
      [observer onLocationError:self.lastLocationStatus];
  }
}

- (void)processHeadingUpdate:(CLHeading *)headingInfo
{
  self.lastHeadingInfo = headingInfo;
  sendInfoToFramework(^{
    GetFramework().OnCompassUpdate(compassInfoFromHeading(self.lastHeadingInfo));
  });
  location::CompassInfo const compassInfo = compassInfoFromHeading(headingInfo);
  for (TObserver observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onHeadingUpdate:)])
      [observer onHeadingUpdate:compassInfo];
  }
}

- (void)processLocationUpdate:(CLLocation *)locationInfo
{
  if (!locationInfo || self.lastLocationStatus != location::TLocationError::ENoError)
    return;
  location::GpsInfo const gpsInfo = gpsInfoFromLocation(locationInfo);
  [self onLocationUpdate:gpsInfo];
  if (self.lastLocationInfo == locationInfo)
    return;
  self.lastLocationInfo = locationInfo;
  [self.predictor reset:gpsInfo];
}

- (void)onLocationUpdate:(location::GpsInfo const &)gpsInfo
{
  GpsTracker::Instance().OnLocationUpdated(gpsInfo);
  sendInfoToFramework([gpsInfo] { GetFramework().OnLocationUpdate(gpsInfo); });
  for (TObserver observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onLocationUpdate:)])
      [observer onLocationUpdate:gpsInfo];
  }
}

#pragma mark - Location Status

- (void)setLastLocationStatus:(location::TLocationError)lastLocationStatus
{
  _lastLocationStatus = lastLocationStatus;
  switch (lastLocationStatus)
  {
  case location::ENoError: break;
  case location::ENotSupported:
    [[MWMAlertViewController activeAlertController] presentLocationServiceNotSupportedAlert];
    break;
  case location::EDenied:
    [[MWMAlertViewController activeAlertController] presentLocationAlert];
    break;
  case location::EGPSIsOff:
    // iOS shows its own alert.
    break;
  }
}

#pragma mark - My Position

+ (void)setMyPositionMode:(location::EMyPositionMode)mode
{
  MWMLocationManager * manager = [MWMLocationManager manager];
  [manager.predictor setMyPositionMode:mode];
  [manager processLocationStatus:manager.lastLocationStatus];
  auto const & f = GetFramework();
  if (f.IsRoutingActive())
  {
    switch (f.GetRouter())
    {
    case routing::RouterType::Vehicle: manager.geoMode = GeoMode::VehicleRouting; break;
    case routing::RouterType::Pedestrian: manager.geoMode = GeoMode::PedestrianRouting; break;
    case routing::RouterType::Bicycle: manager.geoMode = GeoMode::BicycleRouting; break;
    case routing::RouterType::Taxi: break;
    }
  }
  else
  {
    switch (mode)
    {
    case location::EMyPositionMode::PendingPosition: manager.geoMode = GeoMode::Pending; break;
    case location::EMyPositionMode::NotFollowNoPosition:
    case location::EMyPositionMode::NotFollow: manager.geoMode = GeoMode::NotInPosition; break;
    case location::EMyPositionMode::Follow: manager.geoMode = GeoMode::InPosition; break;
    case location::EMyPositionMode::FollowAndRotate:
      manager.geoMode = GeoMode::FollowAndRotate;
      break;
    }
  }
}

#pragma mark - Prediction

- (MWMLocationPredictor *)predictor
{
  if (!_predictor)
  {
    __weak MWMLocationManager * weakSelf = self;
    _predictor = [[MWMLocationPredictor alloc]
        initWithOnPredictionBlock:^(location::GpsInfo const & gpsInfo) {
          [weakSelf onLocationUpdate:gpsInfo];
        }];
  }
  return _predictor;
}

#pragma mark - Device notifications

- (void)orientationChanged
{
  self.locationManager.headingOrientation =
      (CLDeviceOrientation)[UIDevice currentDevice].orientation;
}

- (void)batteryStateChangedNotification:(NSNotification *)notification
{
  [self refreshGeoModeSettings];
}

#pragma mark - Location manager

- (void)setGeoMode:(GeoMode)geoMode
{
  if (_geoMode == geoMode)
    return;
  _geoMode = geoMode;
  CLLocationManager * locationManager = self.locationManager;
  switch (geoMode)
  {
  case GeoMode::Pending:
  case GeoMode::InPosition:
  case GeoMode::NotInPosition:
  case GeoMode::FollowAndRotate: locationManager.activityType = CLActivityTypeOther; break;
  case GeoMode::VehicleRouting:
    locationManager.activityType = CLActivityTypeAutomotiveNavigation;
    break;
  case GeoMode::PedestrianRouting:
  case GeoMode::BicycleRouting: locationManager.activityType = CLActivityTypeFitness; break;
  }
  [self refreshGeoModeSettings];
}

- (void)refreshGeoModeSettings
{
  UIDeviceBatteryState const state = [UIDevice currentDevice].batteryState;
  BOOL const isCharging =
      (state == UIDeviceBatteryStateCharging || state == UIDeviceBatteryStateFull);
  GeoModeSettings const settings = kGeoSettings.at(self.geoMode);
  CLLocationManager * locationManager = self.locationManager;
  locationManager.desiredAccuracy =
      isCharging ? settings.accuracy.charging : settings.accuracy.battery;
  locationManager.distanceFilter = settings.distanceFilter;
}

- (CLLocationManager *)locationManager
{
  if (!_locationManager)
  {
    _locationManager = [[CLLocationManager alloc] init];
    _locationManager.delegate = self;
    [self refreshGeoModeSettings];
    _locationManager.pausesLocationUpdatesAutomatically = YES;
    _locationManager.headingFilter = 3.0;
  }
  return _locationManager;
}

#pragma mark - CLLocationManagerDelegate

- (void)locationManager:(CLLocationManager *)manager didUpdateHeading:(CLHeading *)heading
{
  [self processHeadingUpdate:heading];
}

- (void)locationManager:(CLLocationManager *)manager
     didUpdateLocations:(NSArray<CLLocation *> *)locations
{
  CLLocation * location = locations.lastObject;
  // According to documentation, lat and lon are valid only if horizontalAccuracy is non-negative.
  // So we filter out such events completely.
  if (location.horizontalAccuracy < 0.)
    return;

  self.lastLocationStatus = location::TLocationError::ENoError;
  [self processLocationUpdate:location];
  [[Statistics instance] logLocation:location];
}

- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error
{
  if (self.lastLocationStatus == location::TLocationError::ENoError && error.code == kCLErrorDenied)
    [self processLocationStatus:location::EDenied];
}

#pragma mark - Start / Stop

- (void)setStarted:(BOOL)started
{
  if (_started == started)
    return;
  UIDevice * device = [UIDevice currentDevice];
  NSNotificationCenter * notificationCenter = [NSNotificationCenter defaultCenter];
  if (started)
  {
    _started = [self start];
    device.batteryMonitoringEnabled = YES;
    [notificationCenter addObserver:self
                           selector:@selector(orientationChanged)
                               name:UIDeviceOrientationDidChangeNotification
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(batteryStateChangedNotification:)
                               name:UIDeviceBatteryStateDidChangeNotification
                             object:nil];
  }
  else
  {
    _started = NO;
    [self stop];
    device.batteryMonitoringEnabled = NO;
    [notificationCenter removeObserver:self
                                  name:UIDeviceOrientationDidChangeNotification
                                object:nil];
    [notificationCenter removeObserver:self
                                  name:UIDeviceBatteryStateDidChangeNotification
                                object:nil];
  }
}

- (BOOL)start
{
  auto const doStart = ^{
    LOG(LINFO, ("startUpdatingLocation"));
    CLLocationManager * locationManager = self.locationManager;
    if ([locationManager respondsToSelector:@selector(requestWhenInUseAuthorization)])
      [locationManager requestWhenInUseAuthorization];
    [locationManager startUpdatingLocation];
    setPermissionRequested();
    if ([CLLocationManager headingAvailable])
      [locationManager startUpdatingHeading];
    [[PushNotificationManager pushManager] startLocationTracking];
  };
  if ([CLLocationManager locationServicesEnabled])
  {
    switch ([CLLocationManager authorizationStatus])
    {
    case kCLAuthorizationStatusAuthorizedWhenInUse:
    case kCLAuthorizationStatusAuthorizedAlways:
    case kCLAuthorizationStatusNotDetermined: doStart(); return YES;
    case kCLAuthorizationStatusRestricted:
    case kCLAuthorizationStatusDenied: [self processLocationStatus:location::EDenied]; break;
    }
  }
  else
  {
    // Call start to make iOS show its alert to request geo service.
    doStart();
    [self processLocationStatus:location::EGPSIsOff];
  }
  return NO;
}

- (void)stop
{
  LOG(LINFO, ("stopUpdatingLocation"));
  CLLocationManager * locationManager = self.locationManager;
  [locationManager stopUpdatingLocation];
  if ([CLLocationManager headingAvailable])
    [locationManager stopUpdatingHeading];
  [[PushNotificationManager pushManager] stopLocationTracking];
}

@end
