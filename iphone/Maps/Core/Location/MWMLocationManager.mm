#import "MWMLocationManager.h"
#import <Pushwoosh/PushNotificationManager.h>
#import "MWMAlertViewController.h"
#import "MWMGeoTrackerCore.h"
#import "MWMLocationObserver.h"
#import "MWMLocationPredictor.h"
#import "MWMRouter.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"
#import "SwiftBridge.h"
#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

#include "map/gps_tracker.hpp"

namespace
{
using Observer = id<MWMLocationObserver>;
using Observers = NSHashTable<Observer>;

location::GpsInfo gpsInfoFromLocation(CLLocation * l, location::TLocationSource source)
{
  location::GpsInfo info;
  info.m_source = source;

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
    info.m_speedMpS = l.speed;
  return info;
}

location::CompassInfo compassInfoFromHeading(CLHeading * h)
{
  location::CompassInfo info;
  if (h.trueHeading >= 0.0)
    info.m_bearing = base::DegToRad(h.trueHeading);
  else if (h.headingAccuracy >= 0.0)
    info.m_bearing = base::DegToRad(h.magneticHeading);
  return info;
}

typedef NS_OPTIONS(NSUInteger, MWMLocationFrameworkUpdate) {
  MWMLocationFrameworkUpdateNone = 0,
  MWMLocationFrameworkUpdateLocation = 1 << 0,
  MWMLocationFrameworkUpdateHeading = 1 << 1,
  MWMLocationFrameworkUpdateStatus = 1 << 2
};

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

  auto const isRouteBuilt = [MWMRouter isRouteBuilt];
  auto const isRouteFinished = [MWMRouter isRouteFinished];
  auto const isRouteRebuildingOnly = [MWMRouter isRouteRebuildingOnly];
  auto const needGPSForRouting = ((isRouteBuilt || isRouteRebuildingOnly) && !isRouteFinished);
  if (needGPSForRouting)
    return YES;

  return NO;
}

NSString * const kLocationPermissionRequestedKey = @"kLocationPermissionRequestedKey";

BOOL isPermissionRequested()
{
  return [NSUserDefaults.standardUserDefaults boolForKey:kLocationPermissionRequestedKey];
}

void setPermissionRequested()
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
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
@property(nonatomic) Observers * observers;
@property(nonatomic) MWMLocationFrameworkUpdate frameworkUpdateMode;
@property(nonatomic) location::TLocationSource locationSource;
@property(nonatomic) id<IGeoTracker> geoTracker;

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
  {
    _observers = [Observers weakObjectsHashTable];
    _geoTracker = [Geo geoTracker];
  }
  return self;
}

- (void)dealloc
{
  [NSNotificationCenter.defaultCenter removeObserver:self];
  self.locationManager.delegate = nil;
}
+ (void)start { [self manager].started = YES; }
#pragma mark - Add/Remove Observers

+ (void)addObserver:(Observer)observer
{
  dispatch_async(dispatch_get_main_queue(), ^{
    MWMLocationManager * manager = [self manager];
    [manager.observers addObject:observer];
    [manager processLocationUpdate:manager.lastLocationInfo];
  });
}

+ (void)removeObserver:(Observer)observer
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [[self manager].observers removeObject:observer];
  });
}

#pragma mark - App Life Cycle

+ (void)applicationDidBecomeActive
{
  if (isPermissionRequested() || ![Alohalytics isFirstSession])
  {
    [self start];
    [[self manager] updateFrameworkInfo];
  }
}

+ (void)applicationWillResignActive
{
  BOOL const keepRunning = isPermissionRequested() && keepRunningInBackground();
  MWMLocationManager * manager = [self manager];
  CLLocationManager * locationManager = manager.locationManager;
  if ([locationManager respondsToSelector:@selector(setAllowsBackgroundLocationUpdates:)])
    [locationManager setAllowsBackgroundLocationUpdates:keepRunning];
  manager.started = keepRunning;
}

#pragma mark - Getters

+ (CLLocation *)lastLocation
{
  MWMLocationManager * manager = [self manager];
  if (!manager.started || !manager.lastLocationInfo ||
      manager.lastLocationInfo.horizontalAccuracy < 0 ||
      manager.lastLocationStatus != location::TLocationError::ENoError)
    return nil;
  return manager.lastLocationInfo;
}

+ (BOOL)isLocationProhibited
{
  auto const status = [self manager].lastLocationStatus;
  return status == location::TLocationError::EDenied ||
         status == location::TLocationError::EGPSIsOff;
}

+ (CLHeading *)lastHeading
{
  MWMLocationManager * manager = [self manager];
  if (!manager.started || !manager.lastHeadingInfo || manager.lastHeadingInfo.headingAccuracy < 0)
    return nil;
  return manager.lastHeadingInfo;
}

#pragma mark - Observer notifications

- (void)processLocationStatus:(location::TLocationError)locationError
{
  self.lastLocationStatus = locationError;
  if (self.lastLocationStatus != location::TLocationError::ENoError)
    self.frameworkUpdateMode |= MWMLocationFrameworkUpdateStatus;
  for (Observer observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onLocationError:)])
      [observer onLocationError:self.lastLocationStatus];
  }
}

- (void)processHeadingUpdate:(CLHeading *)headingInfo
{
  self.lastHeadingInfo = headingInfo;
  self.frameworkUpdateMode |= MWMLocationFrameworkUpdateHeading;
  location::CompassInfo const compassInfo = compassInfoFromHeading(headingInfo);
  for (Observer observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onHeadingUpdate:)])
      [observer onHeadingUpdate:compassInfo];
  }
}

- (void)processLocationUpdate:(CLLocation *)locationInfo
{
  if (!locationInfo || self.lastLocationStatus != location::TLocationError::ENoError)
    return;
  [self onLocationUpdate:locationInfo source:self.locationSource];
  if (![self.lastLocationInfo isEqual:locationInfo])
    [self.predictor reset:locationInfo];
}

- (void)onLocationUpdate:(CLLocation *)locationInfo source:(location::TLocationSource)source
{
  location::GpsInfo const gpsInfo = gpsInfoFromLocation(locationInfo, source);
  GpsTracker::Instance().OnLocationUpdated(gpsInfo);

  self.lastLocationInfo = locationInfo;
  self.locationSource = source;
  self.frameworkUpdateMode |= MWMLocationFrameworkUpdateLocation;
  for (Observer observer in self.observers)
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

+ (void)setMyPositionMode:(MWMMyPositionMode)mode
{
  MWMLocationManager * manager = [self manager];
  [manager.predictor setMyPositionMode:mode];
  [manager processLocationStatus:manager.lastLocationStatus];
  if ([MWMRouter isRoutingActive])
  {
    switch ([MWMRouter type])
    {
    case MWMRouterTypeVehicle: manager.geoMode = GeoMode::VehicleRouting; break;
    case MWMRouterTypePublicTransport:
    case MWMRouterTypePedestrian: manager.geoMode = GeoMode::PedestrianRouting; break;
    case MWMRouterTypeBicycle: manager.geoMode = GeoMode::BicycleRouting; break;
    case MWMRouterTypeTaxi: break;
    }
  }
  else
  {
    switch (mode)
    {
    case MWMMyPositionModePendingPosition: manager.geoMode = GeoMode::Pending; break;
    case MWMMyPositionModeNotFollowNoPosition:
    case MWMMyPositionModeNotFollow: manager.geoMode = GeoMode::NotInPosition; break;
    case MWMMyPositionModeFollow: manager.geoMode = GeoMode::InPosition; break;
    case MWMMyPositionModeFollowAndRotate: manager.geoMode = GeoMode::FollowAndRotate; break;
    }
  }
}

#pragma mark - Prediction

- (MWMLocationPredictor *)predictor
{
  if (!_predictor)
  {
    __weak MWMLocationManager * weakSelf = self;
    _predictor = [[MWMLocationPredictor alloc] initWithOnPredictionBlock:^(CLLocation * location) {
      [weakSelf onLocationUpdate:location source:location::EPredictor];
    }];
  }
  return _predictor;
}

#pragma mark - Device notifications

- (void)orientationChanged
{
  self.locationManager.headingOrientation = (CLDeviceOrientation)UIDevice.currentDevice.orientation;
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
  UIDeviceBatteryState const state = UIDevice.currentDevice.batteryState;
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
  self.locationSource = location::EAppleNative;
  [self processLocationUpdate:location];
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
  NSNotificationCenter * notificationCenter = NSNotificationCenter.defaultCenter;
  if (started)
  {
    _started = [self start];
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
  MWMVoidBlock doStart = ^{
    LOG(LINFO, ("startUpdatingLocation"));
    CLLocationManager * locationManager = self.locationManager;
    if ([locationManager respondsToSelector:@selector(requestAlwaysAuthorization)])
      [locationManager requestAlwaysAuthorization];
    [locationManager startUpdatingLocation];
    [self.geoTracker startTracking];
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

#pragma mark - Framework

- (void)updateFrameworkInfo
{
  auto app = UIApplication.sharedApplication;
  if (app.applicationState != UIApplicationStateActive)
    return;
  auto delegate = static_cast<MapsAppDelegate *>(app.delegate);
  if (delegate.isDrapeEngineCreated)
  {
    auto & f = GetFramework();
    if (self.frameworkUpdateMode & MWMLocationFrameworkUpdateLocation)
    {
      location::GpsInfo const gpsInfo =
          gpsInfoFromLocation(self.lastLocationInfo, self.locationSource);
      f.OnLocationUpdate(gpsInfo);
    }
    if (self.frameworkUpdateMode & MWMLocationFrameworkUpdateHeading)
      f.OnCompassUpdate(compassInfoFromHeading(self.lastHeadingInfo));
    if (self.frameworkUpdateMode & MWMLocationFrameworkUpdateStatus)
      f.OnLocationError(self.lastLocationStatus);
    self.frameworkUpdateMode = MWMLocationFrameworkUpdateNone;
  }
  else
  {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self updateFrameworkInfo];
    });
  }
}

#pragma mark - Property

- (void)setFrameworkUpdateMode:(MWMLocationFrameworkUpdate)frameworkUpdateMode
{
  if (frameworkUpdateMode != _frameworkUpdateMode &&
      _frameworkUpdateMode == MWMLocationFrameworkUpdateNone &&
      frameworkUpdateMode != MWMLocationFrameworkUpdateNone)
  {
    _frameworkUpdateMode = frameworkUpdateMode;
    [self updateFrameworkInfo];
  }
  else
  {
    _frameworkUpdateMode = frameworkUpdateMode;
  }
}

@end
