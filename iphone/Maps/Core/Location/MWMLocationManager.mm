#import "MWMLocationManager.h"
#import "MWMAlertViewController.h"
#import "MWMLocationObserver.h"
#import "MWMLocationPredictor.h"
#import "MWMRouter.h"
#import "SwiftBridge.h"
#import "location_util.h"

#include <CoreApi/Framework.h>

#include "map/gps_tracker.hpp"

#if TARGET_OS_SIMULATOR
#include "MountainElevationGenerator.hpp"
#endif

namespace
{
using Observer = id<MWMLocationObserver>;
using Observers = NSHashTable<Observer>;

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

std::string DebugPrint(GeoMode geoMode)
{
  using enum GeoMode;
  switch (geoMode)
  {
    case Pending: return "Pending";
    case InPosition: return "InPosition";
    case NotInPosition: return "NotInPosition";
    case FollowAndRotate: return "FollowAndRotate";
    case VehicleRouting: return "VehicleRouting";
    case PedestrianRouting: return "PedestrianRouting";
    case BicycleRouting: return "BicycleRouting";
  }
  CHECK(false, ("Unsupported value", static_cast<int>(geoMode)));
}

std::string DebugPrint(MWMMyPositionMode mode)
{
  switch (mode)
  {
    case MWMMyPositionModePendingPosition: return "MWMMyPositionModePendingPosition";
    case MWMMyPositionModeNotFollowNoPosition: return "MWMMyPositionModeNotFollowNoPosition";
    case MWMMyPositionModeNotFollow: return "MWMMyPositionModeNotFollow";
    case MWMMyPositionModeFollow: return "MWMMyPositionModeFollow";
    case MWMMyPositionModeFollowAndRotate: return "MWMMyPositionModeFollowAndRotate";
  }
  CHECK(false, ("Unsupported value", static_cast<int>(mode)));
}

std::string DebugPrint(MWMLocationStatus status)
{
  switch (status)
  {
    case MWMLocationStatusNoError: return "MWMLocationStatusNoError";
    case MWMLocationStatusNotSupported: return "MWMLocationStatusNotSupported";
    case MWMLocationStatusDenied: return "MWMLocationStatusDenied";
    case MWMLocationStatusGPSIsOff: return "MWMLocationStatusGPSIsOff";
    case MWMLocationStatusTimeout: return "MWMLocationStatusTimeout";
  }
  CHECK(false, ("Unsupported value", static_cast<int>(status)));
}

std::string DebugPrint(CLAuthorizationStatus status)
{
  switch (status)
  {
    case kCLAuthorizationStatusNotDetermined: return "kCLAuthorizationStatusNotDetermined";
    case kCLAuthorizationStatusRestricted: return "kCLAuthorizationStatusRestricted";
    case kCLAuthorizationStatusDenied: return "kCLAuthorizationStatusDenied";
    case kCLAuthorizationStatusAuthorizedAlways: return "kCLAuthorizationStatusAuthorizedAlways";
    case kCLAuthorizationStatusAuthorizedWhenInUse: return "kCLAuthorizationStatusAuthorizedWhenInUse";
  }
  CHECK(false, ("Unsupported value", static_cast<int>(status)));
}

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
    {          GeoMode::Pending,
     {.distanceFilter = kCLDistanceFilterNone,
     .accuracy = {.charging = kCLLocationAccuracyBestForNavigation, .battery = kCLLocationAccuracyBestForNavigation}}},
    {       GeoMode::InPosition,
     {.distanceFilter = 2,
     .accuracy = {.charging = kCLLocationAccuracyBestForNavigation, .battery = kCLLocationAccuracyBest}}             },
    {    GeoMode::NotInPosition,
     {.distanceFilter = 5,
     .accuracy = {.charging = kCLLocationAccuracyBestForNavigation, .battery = kCLLocationAccuracyBest}}             },
    {  GeoMode::FollowAndRotate,
     {.distanceFilter = 2,
     .accuracy = {.charging = kCLLocationAccuracyBestForNavigation, .battery = kCLLocationAccuracyBest}}             },
    {   GeoMode::VehicleRouting,
     {.distanceFilter = kCLDistanceFilterNone,
     .accuracy = {.charging = kCLLocationAccuracyBestForNavigation, .battery = kCLLocationAccuracyBest}}             },
    {GeoMode::PedestrianRouting,
     {.distanceFilter = 2,
     .accuracy = {.charging = kCLLocationAccuracyBestForNavigation, .battery = kCLLocationAccuracyBest}}             },
    {   GeoMode::BicycleRouting,
     {.distanceFilter = 2,
     .accuracy = {.charging = kCLLocationAccuracyBestForNavigation, .battery = kCLLocationAccuracyBest}}             }
};

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

BOOL needShowLocationAlert()
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  if ([ud objectForKey:kLocationAlertNeedShowKey] == nil)
    return YES;
  return [ud boolForKey:kLocationAlertNeedShowKey];
}

void setShowLocationAlert(BOOL needShow)
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setBool:needShow forKey:kLocationAlertNeedShowKey];
}
}  // namespace

@interface MWMLocationManager () <CLLocationManagerDelegate>

@property(nonatomic) BOOL started;
@property(nonatomic) CLLocationManager * locationManager;
@property(nonatomic) GeoMode geoMode;
@property(nonatomic) CLHeading * lastHeadingInfo;
@property(nonatomic) CLLocation * lastLocationInfo;
@property(nonatomic) MWMLocationStatus lastLocationStatus;
@property(nonatomic) MWMLocationPredictor * predictor;
@property(nonatomic) Observers * observers;
@property(nonatomic) location::TLocationSource locationSource;

@end

@implementation MWMLocationManager

#pragma mark - Init

+ (MWMLocationManager *)manager
{
  static MWMLocationManager * manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ manager = [[self alloc] initManager]; });
  return manager;
}

- (instancetype)initManager
{
  self = [super init];
  if (self)
    _observers = [Observers weakObjectsHashTable];
  return self;
}

- (void)dealloc
{
  [NSNotificationCenter.defaultCenter removeObserver:self];
  self.locationManager.delegate = nil;
}

+ (void)start
{
  [self manager].started = YES;
}

+ (void)stop
{
  [self manager].started = NO;
}

+ (BOOL)isStarted
{
  return [self manager].started;
}

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
  dispatch_async(dispatch_get_main_queue(), ^{ [[self manager].observers removeObject:observer]; });
}

#pragma mark - App Life Cycle

+ (void)applicationDidBecomeActive
{
  [self start];
}

+ (void)applicationWillResignActive
{
  BOOL const keepRunning = keepRunningInBackground();
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
  if (!manager.started || !manager.lastLocationInfo || manager.lastLocationInfo.horizontalAccuracy < 0 ||
      manager.lastLocationStatus != MWMLocationStatusNoError)
    return nil;
  return manager.lastLocationInfo;
}

+ (BOOL)isLocationProhibited
{
  auto const status = [self manager].lastLocationStatus;
  return status == MWMLocationStatusDenied || status == MWMLocationStatusGPSIsOff;
}

+ (CLHeading *)lastHeading
{
  MWMLocationManager * manager = [self manager];
  if (!manager.started || !manager.lastHeadingInfo || manager.lastHeadingInfo.headingAccuracy < 0)
    return nil;
  return manager.lastHeadingInfo;
}

#pragma mark - Observer notifications

- (void)processLocationStatus:(MWMLocationStatus)locationStatus
{
  LOG(LINFO, ("Location status updated from", DebugPrint(self.lastLocationStatus), "to", DebugPrint(locationStatus)));
  self.lastLocationStatus = locationStatus;
  if (self.lastLocationStatus != MWMLocationStatusNoError)
    GetFramework().OnLocationError((location::TLocationError)self.lastLocationStatus);
  for (Observer observer in self.observers)
    if ([observer respondsToSelector:@selector(onLocationError:)])
      [observer onLocationError:self.lastLocationStatus];
}

- (void)processHeadingUpdate:(CLHeading *)headingInfo
{
  self.lastHeadingInfo = headingInfo;
  GetFramework().OnCompassUpdate(location_util::compassInfoFromHeading(headingInfo));
  for (Observer observer in self.observers)
    if ([observer respondsToSelector:@selector(onHeadingUpdate:)])
      [observer onHeadingUpdate:headingInfo];
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
  GetFramework().OnLocationUpdate(gpsInfo);

  self.lastLocationInfo = locationInfo;
  self.locationSource = source;
  for (Observer observer in self.observers)
    if ([observer respondsToSelector:@selector(onLocationUpdate:)])
      [observer onLocationUpdate:locationInfo];
}

#pragma mark - Location Status

- (void)setLastLocationStatus:(MWMLocationStatus)lastLocationStatus
{
  _lastLocationStatus = lastLocationStatus;
  switch (lastLocationStatus)
  {
    case MWMLocationStatusNoError: break;
    case MWMLocationStatusNotSupported:
      [[MWMAlertViewController activeAlertController] presentLocationServiceNotSupportedAlert];
      break;
    case MWMLocationStatusDenied:
      if (needShowLocationAlert())
      {
        [[MWMAlertViewController activeAlertController]
            presentLocationAlertWithCancelBlock:^{ setShowLocationAlert(NO); }];
      }
      break;
    case MWMLocationStatusGPSIsOff:
      if (needShowLocationAlert())
      {
        [[MWMAlertViewController activeAlertController] presentLocationServicesDisabledAlert];
        setShowLocationAlert(NO);
      }
      break;
    case MWMLocationStatusTimeout: CHECK(false, ("MWMLocationStatusTimeout is only used in Qt/Desktop builds"));
  }
}

#pragma mark - My Position

+ (void)setMyPositionMode:(MWMMyPositionMode)mode
{
  LOG(LINFO, ("MyPositionMode updated to", DebugPrint(mode)));
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
      case MWMRouterTypeRuler: break;
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

+ (void)checkLocationStatus
{
  setShowLocationAlert(YES);
  [self.manager processLocationStatus:self.manager.lastLocationStatus];
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
  [MWMLocationManager refreshGeoModeSettingsFor:self.locationManager geoMode:self.geoMode];
}

#pragma mark - Location manager

- (void)setGeoMode:(GeoMode)geoMode
{
  LOG(LINFO, ("GeoMode updated to", geoMode));
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
    case GeoMode::VehicleRouting: locationManager.activityType = CLActivityTypeAutomotiveNavigation; break;
    case GeoMode::PedestrianRouting:
    case GeoMode::BicycleRouting: locationManager.activityType = CLActivityTypeOtherNavigation; break;
  }

  [MWMLocationManager refreshGeoModeSettingsFor:self.locationManager geoMode:self.geoMode];
}

+ (void)refreshGeoModeSettingsFor:(CLLocationManager *)locationManager geoMode:(GeoMode)geoMode
{
  UIDeviceBatteryState const state = UIDevice.currentDevice.batteryState;
  BOOL const isCharging = (state == UIDeviceBatteryStateCharging || state == UIDeviceBatteryStateFull);
  GeoModeSettings const settings = kGeoSettings.at(geoMode);
  locationManager.desiredAccuracy = isCharging ? settings.accuracy.charging : settings.accuracy.battery;
  locationManager.distanceFilter = settings.distanceFilter;
  LOG(LINFO, ("Refreshed GeoMode settings: accuracy", locationManager.desiredAccuracy, "distance filter",
              locationManager.distanceFilter, "charging", isCharging));
}

- (CLLocationManager *)locationManager
{
  if (!_locationManager)
  {
    _locationManager = [[CLLocationManager alloc] init];
    _locationManager.delegate = self;
    [MWMLocationManager refreshGeoModeSettingsFor:_locationManager geoMode:self.geoMode];
    _locationManager.pausesLocationUpdatesAutomatically = NO;
    _locationManager.headingFilter = 3.0;
  }
  return _locationManager;
}

#pragma mark - CLLocationManagerDelegate

- (void)locationManager:(CLLocationManager *)manager didUpdateHeading:(CLHeading *)heading
{
  [self processHeadingUpdate:heading];
}

- (void)locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray<CLLocation *> *)locations
{
  CLLocation * location = locations.lastObject;
  // According to documentation, lat and lon are valid only if horizontalAccuracy is non-negative.
  // So we filter out such events completely.
  if (location.horizontalAccuracy < 0.)
    return;

#if TARGET_OS_SIMULATOR
  // There is no simulator < 15.0 in the new XCode.
  if (@available(iOS 15.0, *))
  {
    // iOS Simulator doesn't provide any elevation in its locations. Mock it.
    static MountainElevationGenerator generator;
    location = [[CLLocation alloc] initWithCoordinate:location.coordinate
                                             altitude:generator.NextElevation()
                                   horizontalAccuracy:location.horizontalAccuracy
                                     verticalAccuracy:location.horizontalAccuracy
                                               course:location.course
                                       courseAccuracy:location.courseAccuracy
                                                speed:location.speed
                                        speedAccuracy:location.speedAccuracy
                                            timestamp:location.timestamp
                                           sourceInfo:location.sourceInformation];
  }
#endif

  self.lastLocationStatus = MWMLocationStatusNoError;
  self.locationSource = location::EAppleNative;
  [self processLocationUpdate:location];
}

- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error
{
  LOG(LWARNING, ("CLLocationManagerDelegate: Did fail with error:", error.localizedDescription.UTF8String));
  if (self.lastLocationStatus == MWMLocationStatusNoError && error.code == kCLErrorDenied)
    [self processLocationStatus:MWMLocationStatusDenied];
}

// Delegate's method didChangeAuthorizationStatus is used to handle the authorization status when the application
// finishes launching or user changes location access in the application settings.
- (void)locationManager:(CLLocationManager *)manager didChangeAuthorizationStatus:(CLAuthorizationStatus)status
{
  LOG(LWARNING, ("CLLocationManagerDelegate: Authorization status has changed to", DebugPrint(status)));
  switch (status)
  {
    case kCLAuthorizationStatusAuthorizedWhenInUse:
    case kCLAuthorizationStatusAuthorizedAlways: [self startUpdatingLocationFor:manager]; break;
    case kCLAuthorizationStatusNotDetermined: [manager requestWhenInUseAuthorization]; break;
    case kCLAuthorizationStatusRestricted:
    case kCLAuthorizationStatusDenied:
      if ([CLLocationManager locationServicesEnabled])
        [self processLocationStatus:MWMLocationStatusDenied];
      else
        [self processLocationStatus:MWMLocationStatusGPSIsOff];
      break;
  }
}

- (void)locationManagerDidPauseLocationUpdates:(CLLocationManager *)manager
{
  LOG(LINFO, ("CLLocationManagerDelegate: Location updates were paused"));
}

- (void)locationManagerDidResumeLocationUpdates:(CLLocationManager *)manager
{
  LOG(LINFO, ("CLLocationManagerDelegate: Location updates were resumed"));
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
    if (_started)
    {
      [notificationCenter addObserver:self
                             selector:@selector(orientationChanged)
                                 name:UIDeviceOrientationDidChangeNotification
                               object:nil];
      [notificationCenter addObserver:self
                             selector:@selector(batteryStateChangedNotification:)
                                 name:UIDeviceBatteryStateDidChangeNotification
                               object:nil];
    }
  }
  else
  {
    _started = NO;
    [self stop];
    [notificationCenter removeObserver:self];
  }
}

- (void)startUpdatingLocationFor:(CLLocationManager *)manager
{
  LOG(LINFO, ("Start updating location"));
  [manager startUpdatingLocation];
  if ([CLLocationManager headingAvailable])
    [manager startUpdatingHeading];
}

- (BOOL)start
{
  if ([CLLocationManager locationServicesEnabled])
  {
    CLLocationManager * locationManager = self.locationManager;
    switch (CLLocationManager.authorizationStatus)
    {
      case kCLAuthorizationStatusAuthorizedWhenInUse:
      case kCLAuthorizationStatusAuthorizedAlways:
        [self startUpdatingLocationFor:locationManager];
        return YES;
        break;
      case kCLAuthorizationStatusNotDetermined:
        [locationManager requestWhenInUseAuthorization];
        return YES;
        break;
      case kCLAuthorizationStatusRestricted:
      case kCLAuthorizationStatusDenied: break;
    }
  }
  return NO;
}

- (void)stop
{
  LOG(LINFO, ("Stop updating location"));
  CLLocationManager * locationManager = self.locationManager;
  [locationManager stopUpdatingLocation];
  if ([CLLocationManager headingAvailable])
    [locationManager stopUpdatingHeading];
}

#pragma mark - Location alert

+ (void)enableLocationAlert
{
  setShowLocationAlert(YES);
}

#pragma mark - Helpers

@end
