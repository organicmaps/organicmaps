#import "MWMLocationManager.h"
#import "MWMAlertViewController.h"
#import "MWMLocationObserver.h"
#import "MWMLocationPredictor.h"
#import "MWMRouter.h"
#import "MapsAppDelegate.h"
#import "SwiftBridge.h"
#import "location_util.h"

#include <CoreApi/Framework.h>

#include "map/gps_tracker.hpp"

namespace
{
using Observer = id<MWMLocationObserver>;
using Observers = NSHashTable<Observer>;

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

std::map<GeoMode, GeoModeSettings> const kGeoSettings{
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
  if (GpsTracker::Instance().IsEnabled())
    return YES;

  auto const isOnRoute = [MWMRouter isOnRoute];
  auto const isRouteFinished = [MWMRouter isRouteFinished];
  if (isOnRoute && !isRouteFinished)
    return YES;

  return NO;
}

NSString * const kLocationPermissionRequestedKey = @"kLocationPermissionRequestedKey";
NSString * const kLocationAlertNeedShowKey = @"kLocationAlertNeedShowKey";

BOOL isPermissionRequested() {
  return [NSUserDefaults.standardUserDefaults boolForKey:kLocationPermissionRequestedKey];
}

void setPermissionRequested() {
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setBool:YES forKey:kLocationPermissionRequestedKey];
  [ud synchronize];
}
       
BOOL needShowLocationAlert() {
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  if ([ud objectForKey:kLocationAlertNeedShowKey] == nil)
    return YES;
  return [ud boolForKey:kLocationAlertNeedShowKey];
}

void setShowLocationAlert(BOOL needShow) {
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setBool:needShow forKey:kLocationAlertNeedShowKey];
  [ud synchronize];
}
}  // namespace

@interface MWMLocationManager ()<CLLocationManagerDelegate>

@property(nonatomic) BOOL started;
@property(nonatomic) CLLocationManager * locationManager;
@property(nonatomic) GeoMode geoMode;
@property(nonatomic) CLHeading * lastHeadingInfo;
@property(nonatomic) CLLocation * lastLocationInfo;
@property(nonatomic) MWMLocationStatus lastLocationStatus;
@property(nonatomic) MWMLocationPredictor * predictor;
@property(nonatomic) Observers * observers;
@property(nonatomic) MWMLocationFrameworkUpdate frameworkUpdateMode;
@property(nonatomic) location::TLocationSource locationSource;

@end

@implementation MWMLocationManager

#pragma mark - Init

+ (MWMLocationManager *)manager
{
  static MWMLocationManager * manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[self alloc] initManager];
  });
  return manager;
}

- (instancetype)initManager
{
  self = [super init];
  if (self)
  {
    _observers = [Observers weakObjectsHashTable];
  }
  return self;
}

- (void)dealloc
{
  [NSNotificationCenter.defaultCenter removeObserver:self];
  self.locationManager.delegate = nil;
}

+ (void)start { [self manager].started = YES; }

+ (void)stop { [self manager].started = NO; }

+ (BOOL)isStarted { return [self manager].started; }

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
  if (isPermissionRequested() || ![FirstSession isFirstSession])
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
      manager.lastLocationStatus != MWMLocationStatusNoError)
    return nil;
  return manager.lastLocationInfo;
}

+ (BOOL)isLocationProhibited
{
  auto const status = [self manager].lastLocationStatus;
  return status == MWMLocationStatusDenied ||
         status == MWMLocationStatusGPSIsOff;
}

+ (CLHeading *)lastHeading
{
  MWMLocationManager * manager = [self manager];
  if (!manager.started || !manager.lastHeadingInfo || manager.lastHeadingInfo.headingAccuracy < 0)
    return nil;
  return manager.lastHeadingInfo;
}

#pragma mark - Observer notifications

- (void)processLocationStatus:(MWMLocationStatus)locationError
{
  self.lastLocationStatus = locationError;
  if (self.lastLocationStatus != MWMLocationStatusNoError)
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
//  location::CompassInfo const compassInfo = compassInfoFromHeading(headingInfo);
  for (Observer observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onHeadingUpdate:)])
      [observer onHeadingUpdate:headingInfo];
  }
}

- (void)processLocationUpdate:(CLLocation *)locationInfo
{
  if (!locationInfo || self.lastLocationStatus != MWMLocationStatusNoError)
    return;
  [self onLocationUpdate:locationInfo source:self.locationSource];
  if (![self.lastLocationInfo isEqual:locationInfo])
    [self.predictor reset:locationInfo];
}

- (void)onLocationUpdate:(CLLocation *)locationInfo source:(location::TLocationSource)source
{
  location::GpsInfo const gpsInfo = location_util::gpsInfoFromLocation(locationInfo, source);
  GpsTracker::Instance().OnLocationUpdated(gpsInfo);

  self.lastLocationInfo = locationInfo;
  self.locationSource = source;
  self.frameworkUpdateMode |= MWMLocationFrameworkUpdateLocation;
  for (Observer observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onLocationUpdate:)])
      [observer onLocationUpdate:locationInfo];
  }
}

#pragma mark - Location Status

- (void)setLastLocationStatus:(MWMLocationStatus)lastLocationStatus
{
  _lastLocationStatus = lastLocationStatus;
  switch (lastLocationStatus)
  {
  case MWMLocationStatusNoError:
    break;
  case MWMLocationStatusNotSupported:
    [[MWMAlertViewController activeAlertController] presentLocationServiceNotSupportedAlert];
    break;
  case MWMLocationStatusDenied:
  case MWMLocationStatusGPSIsOff:
    if (needShowLocationAlert()) {
      [[MWMAlertViewController activeAlertController] presentLocationAlertWithCancelBlock:^{
        setShowLocationAlert(NO);
      }];
    }
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

    // FIXME: this should probably be corrected when implementing helicopter routing.
    case MWMRouterTypeHelicopter:
        break;
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
  case GeoMode::FollowAndRotate:
    locationManager.activityType = CLActivityTypeOther;
    break;
  case GeoMode::VehicleRouting:
    locationManager.activityType = CLActivityTypeAutomotiveNavigation;
    break;
  case GeoMode::PedestrianRouting:
  case GeoMode::BicycleRouting:
    locationManager.activityType = CLActivityTypeFitness;
    break;
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

  self.lastLocationStatus = MWMLocationStatusNoError;
  self.locationSource = location::EAppleNative;
  [self processLocationUpdate:location];
}

- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error
{
  if (self.lastLocationStatus == MWMLocationStatusNoError && error.code == kCLErrorDenied)
    [self processLocationStatus:MWMLocationStatusDenied];
}

#pragma mark - Start / Stop

- (void)setStarted:(BOOL)started
{
  if (_started == started)
    return;
  NSNotificationCenter * notificationCenter = NSNotificationCenter.defaultCenter;
  if (started) {
    _started = [self start];
    if (_started) {
      [notificationCenter addObserver:self
                             selector:@selector(orientationChanged)
                                 name:UIDeviceOrientationDidChangeNotification
                               object:nil];
      [notificationCenter addObserver:self
                             selector:@selector(batteryStateChangedNotification:)
                                 name:UIDeviceBatteryStateDidChangeNotification
                               object:nil];
    }
  } else {
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
    if ([locationManager respondsToSelector:@selector(requestWhenInUseAuthorization)])
      [locationManager requestWhenInUseAuthorization];

    [locationManager startUpdatingLocation];

    setPermissionRequested();

    if ([CLLocationManager headingAvailable])
      [locationManager startUpdatingHeading];
  };

  if ([CLLocationManager locationServicesEnabled])
  {
    switch ([CLLocationManager authorizationStatus])
    {
    case kCLAuthorizationStatusAuthorizedWhenInUse:
    case kCLAuthorizationStatusAuthorizedAlways:
    case kCLAuthorizationStatusNotDetermined:
        doStart();
        return YES;
    case kCLAuthorizationStatusRestricted:
    case kCLAuthorizationStatusDenied:
        [self processLocationStatus:MWMLocationStatusDenied];
        break;
    }
  }
  else
  {
    [self processLocationStatus:MWMLocationStatusGPSIsOff];
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
          location_util::gpsInfoFromLocation(self.lastLocationInfo, self.locationSource);
      f.OnLocationUpdate(gpsInfo);
    }
    if (self.frameworkUpdateMode & MWMLocationFrameworkUpdateHeading)
      f.OnCompassUpdate(location_util::compassInfoFromHeading(self.lastHeadingInfo));
    if (self.frameworkUpdateMode & MWMLocationFrameworkUpdateStatus)
      f.OnLocationError((location::TLocationError)self.lastLocationStatus);
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

#pragma mark - Location alert

+ (void)enableLocationAlert {
  setShowLocationAlert(YES);
}

@end
