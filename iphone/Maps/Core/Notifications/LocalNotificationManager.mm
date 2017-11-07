#import "LocalNotificationManager.h"
#import "CLLocation+Mercator.h"
#import "MWMStorage.h"
#import "MapViewController.h"
#import "Statistics.h"
#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

#include "storage/country_info_getter.hpp"
#include "storage/storage_helpers.hpp"

namespace
{
NSString * const kDownloadMapActionKey = @"DownloadMapActionKey";
NSString * const kDownloadMapActionName = @"DownloadMapActionName";
NSString * const kDownloadMapCountryId = @"DownloadMapCountryId";

NSString * const kFlagsKey = @"DownloadMapNotificationFlags";

NSTimeInterval constexpr kRepeatedNotificationIntervalInSeconds =
    3 * 30 * 24 * 60 * 60;  // three months
}  // namespace

using namespace storage;

@interface LocalNotificationManager ()<CLLocationManagerDelegate, UIAlertViewDelegate>

@property(nonatomic) CLLocationManager * locationManager;
@property(copy, nonatomic) CompletionHandler downloadMapCompletionHandler;
@property(weak, nonatomic) NSTimer * timer;

@end

@implementation LocalNotificationManager

+ (instancetype)sharedManager
{
  static LocalNotificationManager * manager = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[self alloc] init];
  });
  return manager;
}

- (void)dealloc { _locationManager.delegate = nil; }
- (void)processNotification:(UILocalNotification *)notification onLaunch:(BOOL)onLaunch
{
  NSDictionary * userInfo = [notification userInfo];
  if ([userInfo[kDownloadMapActionKey] isEqualToString:kDownloadMapActionName])
  {
    [Statistics logEvent:@"'Download Map' Notification Clicked"];
    MapViewController * mapViewController = [MapViewController controller];
    [mapViewController.navigationController popToRootViewControllerAnimated:NO];

    NSString * notificationCountryId = userInfo[kDownloadMapCountryId];
    TCountryId const countryId = notificationCountryId.UTF8String;
    [Statistics logEvent:kStatDownloaderMapAction
          withParameters:@{
            kStatAction : kStatDownload,
            kStatIsAuto : kStatNo,
            kStatFrom : kStatMap,
            kStatScenario : kStatDownload
          }];
    [MWMStorage downloadNode:countryId
                   onSuccess:^{
                     GetFramework().ShowNode(countryId);
                   }];
  }
}

#pragma mark - Location Notifications

- (void)showDownloadMapNotificationIfNeeded:(CompletionHandler)completionHandler
{
  NSTimeInterval const completionTimeIndent = 2.0;
  NSTimeInterval const backgroundTimeRemaining =
      UIApplication.sharedApplication.backgroundTimeRemaining - completionTimeIndent;
  if ([CLLocationManager locationServicesEnabled] && backgroundTimeRemaining > 0.0)
  {
    self.downloadMapCompletionHandler = completionHandler;
    self.timer = [NSTimer scheduledTimerWithTimeInterval:backgroundTimeRemaining
                                                  target:self
                                                selector:@selector(timerSelector:)
                                                userInfo:nil
                                                 repeats:NO];
    LOG(LINFO, ("startUpdatingLocation"));
    [self.locationManager startUpdatingLocation];
  }
  else
  {
    LOG(LINFO, ("stopUpdatingLocation"));
    [self.locationManager stopUpdatingLocation];
    completionHandler(UIBackgroundFetchResultFailed);
  }
}

- (BOOL)shouldShowNotificationForCountryId:(NSString *)countryId
{
  if (!countryId || countryId.length == 0)
    return NO;
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  NSDictionary<NSString *, NSDate *> * flags = [ud objectForKey:kFlagsKey];
  NSDate * lastShowDate = flags[countryId];
  return !lastShowDate ||
         [[NSDate date] timeIntervalSinceDate:lastShowDate] >
             kRepeatedNotificationIntervalInSeconds;
}

- (void)markNotificationShownForCountryId:(NSString *)countryId
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  NSMutableDictionary<NSString *, NSDate *> * flags = [[ud objectForKey:kFlagsKey] mutableCopy];
  if (!flags)
    flags = [NSMutableDictionary dictionary];
  flags[countryId] = [NSDate date];
  [ud setObject:flags forKey:kFlagsKey];
  [ud synchronize];
}

- (void)timerSelector:(id)sender
{
  // Location still was not received but it's time to finish up so system will not kill us.
  LOG(LINFO, ("stopUpdatingLocation"));
  [self.locationManager stopUpdatingLocation];
  [self performCompletionHandler:UIBackgroundFetchResultFailed];
}

- (void)performCompletionHandler:(UIBackgroundFetchResult)result
{
  if (!self.downloadMapCompletionHandler)
    return;
  self.downloadMapCompletionHandler(result);
  self.downloadMapCompletionHandler = nil;
}

#pragma mark - Location Manager

- (CLLocationManager *)locationManager
{
  if (!_locationManager)
  {
    _locationManager = [[CLLocationManager alloc] init];
    _locationManager.delegate = self;
    _locationManager.distanceFilter = kCLLocationAccuracyThreeKilometers;
  }
  return _locationManager;
}

- (void)locationManager:(CLLocationManager *)manager
     didUpdateLocations:(NSArray<CLLocation *> *)locations
{
  [self.timer invalidate];
  LOG(LINFO, ("stopUpdatingLocation"));
  [self.locationManager stopUpdatingLocation];
  NSString * flurryEventName = @"'Download Map' Notification Didn't Schedule";
  UIBackgroundFetchResult result = UIBackgroundFetchResultNoData;

  BOOL const inBackground =
      UIApplication.sharedApplication.applicationState == UIApplicationStateBackground;
  BOOL const onWiFi = (Platform::ConnectionStatus() == Platform::EConnectionType::CONNECTION_WIFI);
  if (inBackground && onWiFi)
  {
    CLLocation * lastLocation = locations.lastObject;
    auto const & mercator = lastLocation.mercator;
    auto & f = GetFramework();
    auto const & countryInfoGetter = f.GetCountryInfoGetter();
    if (!IsPointCoveredByDownloadedMaps(mercator, f.GetStorage(), countryInfoGetter))
    {
      NSString * countryId = @(countryInfoGetter.GetRegionCountryId(mercator).c_str());
      if ([self shouldShowNotificationForCountryId:countryId])
      {
        [self markNotificationShownForCountryId:countryId];

        UILocalNotification * notification = [[UILocalNotification alloc] init];
        notification.alertAction = L(@"download");
        notification.alertBody = L(@"download_map_notification");
        notification.soundName = UILocalNotificationDefaultSoundName;
        notification.userInfo =
            @{kDownloadMapActionKey : kDownloadMapActionName, kDownloadMapCountryId : countryId};

        UIApplication * application = UIApplication.sharedApplication;
        [application presentLocalNotificationNow:notification];

        [Alohalytics logEvent:@"suggestedToDownloadMissingMapForCurrentLocation"
                   atLocation:lastLocation];
        flurryEventName = @"'Download Map' Notification Scheduled";
        result = UIBackgroundFetchResultNewData;
      }
    }
  }
  [Statistics logEvent:flurryEventName withParameters:@{ @"WiFi" : @(onWiFi) }];
  [self performCompletionHandler:result];
}

@end
